#include "MyPlayerController.h"

#include "Animation/MorphTarget.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogInBang, Log, All);

AMyPlayerController::AMyPlayerController()
{
	bShowMouseCursor = false;
	// 폰 빙의 시 뷰타겟이 폰으로 돌아가지 않도록 직접 관리
	bAutoManageActiveCameraTarget = false;
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FindTalent();
	SetupBroadcastCamera();

	if (bSpawnGreenScreenBackdrop)
	{
		SpawnBackdrop();
	}

	ApplyExpression(1.f, 0.f);
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AMyPlayerController::HandleNeutralExpression);
	InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AMyPlayerController::HandleAltExpression);
}

void AMyPlayerController::FindTalent()
{
	UWorld* World = GetWorld();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TalentActorTag))
		{
			TalentActor = *It;
			break;
		}
	}

	if (!TalentActor)
	{
		// 태그가 없으면: 내 폰이 아니면서 실제 메시가 붙은 첫 캐릭터
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			if (*It == GetPawn())
			{
				continue;
			}

			const USkeletalMeshComponent* Mesh = It->GetMesh();
			if (Mesh && Mesh->GetSkeletalMeshAsset())
			{
				TalentActor = *It;
				break;
			}
		}
	}

	if (TalentActor)
	{
		UE_LOG(LogInBang, Log, TEXT("방송 주인공: %s"), *TalentActor->GetName());
	}
	else
	{
		UE_LOG(LogInBang, Warning, TEXT("방송 주인공을 찾지 못했습니다. 레벨에 캐릭터를 배치하거나 태그 '%s'를 붙여주세요."), *TalentActorTag.ToString());
	}
}

USkeletalMeshComponent* AMyPlayerController::GetTalentMesh() const
{
	return TalentActor ? TalentActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

FVector AMyPlayerController::GetFocusLocation() const
{
	if (const USkeletalMeshComponent* Mesh = GetTalentMesh())
	{
		const FBoxSphereBounds& Bounds = Mesh->Bounds;
		const double BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
		return FVector(Bounds.Origin.X, Bounds.Origin.Y, BottomZ + 2.0 * Bounds.BoxExtent.Z * CameraFocusHeightRatio);
	}

	return TalentActor ? TalentActor->GetActorLocation() : FVector::ZeroVector;
}

void AMyPlayerController::SetupBroadcastCamera()
{
	UWorld* World = GetWorld();

	// 레벨에 이미 있는 카메라 우선: 태그 달린 것 > 아무 카메라
	ACameraActor* Tagged = nullptr;
	ACameraActor* First = nullptr;
	for (TActorIterator<ACameraActor> It(World); It; ++It)
	{
		if (!First)
		{
			First = *It;
		}
		if (It->ActorHasTag(CameraActorTag))
		{
			Tagged = *It;
			break;
		}
	}

	if (ACameraActor* Existing = Tagged ? Tagged : First)
	{
		BroadcastCamera = Cast<ACineCameraActor>(Existing);
		SetViewTargetWithBlend(Existing);
		UE_LOG(LogInBang, Log, TEXT("레벨의 카메라 사용: %s"), *Existing->GetName());
		return;
	}

	if (!TalentActor)
	{
		return;
	}

	// 카메라가 없으면 버스트샷으로 자동 생성
	const FVector Focus = GetFocusLocation();
	const FVector Forward = TalentActor->GetActorForwardVector();
	const FVector CameraLocation = Focus + Forward * CameraDistance;
	const FRotator CameraRotation = (Focus - CameraLocation).Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(CameraLocation, CameraRotation, Params);
	if (!Camera)
	{
		return;
	}

	if (UCineCameraComponent* CineComp = Camera->GetCineCameraComponent())
	{
		CineComp->SetCurrentFocalLength(CameraFocalLength);
		CineComp->FocusSettings.FocusMethod = ECameraFocusMethod::Disable;
	}

	Camera->Tags.Add(CameraActorTag);
	BroadcastCamera = Camera;
	SetViewTargetWithBlend(Camera);
	UE_LOG(LogInBang, Log, TEXT("방송 카메라 자동 생성 (거리 %.0fcm, %.0fmm)"), CameraDistance, CameraFocalLength);
}

void AMyPlayerController::SpawnBackdrop()
{
	UWorld* World = GetWorld();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(BackdropActorTag))
		{
			return; // 이미 배경이 있음
		}
	}

	if (!TalentActor)
	{
		return;
	}

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!PlaneMesh)
	{
		return;
	}

	const FVector Focus = GetFocusLocation();
	const FVector Forward = TalentActor->GetActorForwardVector();
	const FVector Location = Focus - Forward * BackdropDistance;
	// 판의 노멀(+Z)이 카메라 쪽을 향하도록
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Forward).Rotator();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Backdrop = World->SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
	if (!Backdrop)
	{
		return;
	}

	Backdrop->SetMobility(EComponentMobility::Movable);
	Backdrop->SetActorScale3D(FVector(100.f, 100.f, 1.f));
	Backdrop->Tags.Add(BackdropActorTag);

	UStaticMeshComponent* MeshComp = Backdrop->GetStaticMeshComponent();
	MeshComp->SetStaticMesh(PlaneMesh);
	MeshComp->SetCastShadow(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 전용 머티리얼이 있으면 사용, 없으면 엔진 기본 머티리얼에 색만 지정
	UMaterialInterface* Material = Cast<UMaterialInterface>(BackdropMaterialPath.TryLoad());
	if (!Material)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Base)
		{
			UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(Base, Backdrop);
			Dynamic->SetVectorParameterValue(TEXT("Color"), BackdropColor);
			Material = Dynamic;
		}
	}
	if (Material)
	{
		MeshComp->SetMaterial(0, Material);
	}

	UE_LOG(LogInBang, Log, TEXT("그린스크린 배경 자동 생성 (%.0fcm 뒤)"), BackdropDistance);
}

void AMyPlayerController::ApplyExpression(float NeutralWeight, float AltWeight)
{
	USkeletalMeshComponent* Mesh = GetTalentMesh();
	if (!Mesh)
	{
		return;
	}

	if (!bWarnedMissingMorphs)
	{
		if (const USkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset())
		{
			const bool bHasNeutral = Asset->FindMorphTarget(NeutralMorphTargetName) != nullptr;
			const bool bHasAlt = Asset->FindMorphTarget(AltMorphTargetName) != nullptr;
			if (!bHasNeutral || !bHasAlt)
			{
				bWarnedMissingMorphs = true;
				UE_LOG(LogInBang, Warning,
					TEXT("모프타겟 '%s'/'%s'가 %s에 없습니다. 콘솔(~)에서 ListMorphs로 실제 이름을 확인한 뒤 DefaultGame.ini를 수정하세요."),
					*NeutralMorphTargetName.ToString(), *AltMorphTargetName.ToString(), *Asset->GetName());
			}
		}
	}

	Mesh->SetMorphTarget(NeutralMorphTargetName, NeutralWeight);
	Mesh->SetMorphTarget(AltMorphTargetName, AltWeight);
}

void AMyPlayerController::RefreshCameraTransform()
{
	if (!BroadcastCamera || !TalentActor)
	{
		return;
	}

	const FVector Focus = GetFocusLocation();
	const FVector Forward = TalentActor->GetActorForwardVector();
	const FVector CameraLocation = Focus + Forward * CameraDistance;
	BroadcastCamera->SetActorLocationAndRotation(CameraLocation, (Focus - CameraLocation).Rotation());
}

void AMyPlayerController::HandleNeutralExpression()
{
	ApplyExpression(1.f, 0.f);
	OnNeutralExpressionRequested();
}

void AMyPlayerController::HandleAltExpression()
{
	ApplyExpression(0.f, 1.f);
	OnAltExpressionRequested();
}

void AMyPlayerController::ListMorphs()
{
	const USkeletalMeshComponent* Mesh = GetTalentMesh();
	const USkeletalMesh* Asset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
	if (!Asset)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, TEXT("ListMorphs: 방송 주인공 메시를 찾지 못했습니다"));
		}
		return;
	}

	const TArray<TObjectPtr<UMorphTarget>>& Morphs = Asset->GetMorphTargets();
	UE_LOG(LogInBang, Log, TEXT("%s 모프타겟 %d개:"), *Asset->GetName(), Morphs.Num());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green,
			FString::Printf(TEXT("%s 모프타겟 %d개 (전체 목록은 로그 참고)"), *Asset->GetName(), Morphs.Num()));
	}

	for (const UMorphTarget* Morph : Morphs)
	{
		if (!Morph)
		{
			continue;
		}
		UE_LOG(LogInBang, Log, TEXT("  %s"), *Morph->GetName());
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, Morph->GetName());
		}
	}
}

void AMyPlayerController::Expr(FName MorphTarget, float Value)
{
	if (USkeletalMeshComponent* Mesh = GetTalentMesh())
	{
		Mesh->SetMorphTarget(MorphTarget, Value);
	}
}

void AMyPlayerController::CamDist(float Distance)
{
	CameraDistance = Distance;
	RefreshCameraTransform();
}

void AMyPlayerController::CamHeight(float Ratio)
{
	CameraFocusHeightRatio = Ratio;
	RefreshCameraTransform();
}

void AMyPlayerController::CamFocal(float FocalLength)
{
	CameraFocalLength = FocalLength;
	if (BroadcastCamera)
	{
		if (UCineCameraComponent* CineComp = BroadcastCamera->GetCineCameraComponent())
		{
			CineComp->SetCurrentFocalLength(CameraFocalLength);
		}
	}
}
