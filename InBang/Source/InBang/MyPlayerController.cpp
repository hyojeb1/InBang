#include "MyPlayerController.h"

#include "Animation/MorphTarget.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/InputComponent.h"
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
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogInBang, Log, All);

namespace
{
	// 액터의 스켈레탈 메시 컴포넌트 중 실제 메시 에셋이 있는 첫 번째를 반환.
	// (BP_Nefer는 캐릭터 기본 Mesh가 아닌 자식 컴포넌트에 메시가 붙어있다)
	USkeletalMeshComponent* FindMeshWithAsset(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TInlineComponentArray<USkeletalMeshComponent*> Components(Actor);
		for (USkeletalMeshComponent* Component : Components)
		{
			if (Component && Component->GetSkeletalMeshAsset())
			{
				return Component;
			}
		}
		return nullptr;
	}
}

AMyPlayerController::AMyPlayerController()
{
	bShowMouseCursor = false;
	// 폰 빙의 시 뷰타겟이 폰으로 돌아가지 않도록 직접 관리
	bAutoManageActiveCameraTarget = false;

	// F1부터 순서대로. DefaultGame.ini의 +ExpressionSlots가 있으면 그쪽이 우선
	ExpressionSlots = {
		TEXT("None"),                                               // 1키 : 기본
		TEXT("Eye_Smile_Closed_L,Eye_Smile_Closed_R,Mouth_Smile"),  // 2키 : 스마일
		TEXT("Eye_Joy_L,Eye_Joy_R,Mouth_joy"),                      // 3키 : 기쁨
		TEXT("Eye_Star_L,Eye_Star_R,Mouth_A"),                      // 4키 : 반짝
		TEXT("Brow_Angry_L,Brow_Angry_R,Mouth_Angry"),              // 5키 : 화남
		TEXT("Brow_Sad_L,Brow_Sad_R,Mouth_Sad"),                    // 6키 : 슬픔
	};
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SetupStage();

	// 다른 액터의 초기화가 아직 안 끝났을 수 있으니 못 찾았으면 한 번 재시도
	if (!TalentActor)
	{
		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, this, &AMyPlayerController::SetupStage, 0.5f, false);
	}
}

//! \brief 언리얼 엔진의 F1
//void AMyPlayerController::SetupInputComponent()
//{
//	Super::SetupInputComponent();
//
//	const int32 NumSlots = FMath::Min(ExpressionSlots.Num(), 12);
//	for (int32 Index = 0; Index < NumSlots; ++Index)
//	{
//		const FKey Key(*FString::Printf(TEXT("F%d"), Index + 1));
//		FInputKeyBinding Binding(FInputChord(Key), IE_Pressed);
//		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, Index]()
//		{
//			Face(Index);
//		});
//		InputComponent->KeyBindings.Add(MoveTemp(Binding));
//	}
//}

//! \brief 언리얼 엔진의 넘패드 키
void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 언리얼 엔진의 넘패드 키 정의를 배열로 미리 준비합니다.
	const TArray<FKey> NumPadKeys = {
		EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
		EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine
	};

	// 표정 개수와 넘패드 키 개수(10개) 중 더 적은 값만큼 루프
	const int32 NumSlots = FMath::Min(ExpressionSlots.Num(), NumPadKeys.Num());

	for (int32 Index = 0; Index < NumSlots; ++Index)
	{
		FInputKeyBinding Binding(FInputChord(NumPadKeys[Index]), IE_Pressed);
		Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, Index]()
			{
				Face(Index);
			});
		InputComponent->KeyBindings.Add(MoveTemp(Binding));
	}
}

void AMyPlayerController::SetupStage()
{
	if (!TalentActor)
	{
		FindTalent();
	}

	if (!bCameraReady)
	{
		SetupBroadcastCamera();
	}

	if (bSpawnGreenScreenBackdrop && !bBackdropReady && TalentActor)
	{
		SpawnBackdrop();
		bBackdropReady = true;
	}
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

			if (FindMeshWithAsset(*It))
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
	return FindMeshWithAsset(TalentActor);
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
		bCameraReady = true;
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
	bCameraReady = true;
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

void AMyPlayerController::Face(int32 SlotIndex)
{
	if (!ExpressionSlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogInBang, Warning, TEXT("표정 슬롯 %d 없음 (0~%d)"), SlotIndex, ExpressionSlots.Num() - 1);
		return;
	}

	ApplyExpressionString(ExpressionSlots[SlotIndex]);
	UE_LOG(LogInBang, Log, TEXT("표정 슬롯 %d 적용: %s (모프 %d개)"), SlotIndex, *ExpressionSlots[SlotIndex], TouchedMorphs.Num());
	OnExpressionChanged(SlotIndex);
}

void AMyPlayerController::ApplyExpressionString(const FString& Expression)
{
	USkeletalMeshComponent* Mesh = GetTalentMesh();
	if (!Mesh)
	{
		UE_LOG(LogInBang, Warning, TEXT("표정 적용 실패: 방송 주인공 메시 없음"));
		return;
	}

	// 이전 표정 리셋
	for (const FName& Touched : TouchedMorphs)
	{
		Mesh->SetMorphTarget(Touched, 0.f);
	}
	TouchedMorphs.Reset();

	const USkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();

	TArray<FString> Tokens;
	Expression.ParseIntoArray(Tokens, TEXT(","));
	for (FString& Token : Tokens)
	{
		Token.TrimStartAndEndInline();
		if (Token.IsEmpty() || Token.Equals(TEXT("None"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FString NamePart = Token;
		FString WeightPart;
		float Weight = 1.f;
		if (Token.Split(TEXT("="), &NamePart, &WeightPart))
		{
			NamePart.TrimStartAndEndInline();
			Weight = FCString::Atof(*WeightPart);
		}

		const FName MorphName(*NamePart);
		if (Asset && !Asset->FindMorphTarget(MorphName))
		{
			UE_LOG(LogInBang, Warning, TEXT("모프타겟 '%s'가 %s에 없습니다. 콘솔(~)에서 ListMorphs로 확인하세요."),
				*NamePart, *Asset->GetName());
			continue;
		}

		Mesh->SetMorphTarget(MorphName, Weight);
		TouchedMorphs.Add(MorphName);
	}
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
		TouchedMorphs.Add(MorphTarget);
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
