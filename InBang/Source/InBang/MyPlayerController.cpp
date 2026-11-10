#include "MyPlayerController.h"

#include "Animation/MorphTarget.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Widgets/SWindow.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogInBang, Log, All);

namespace
{
	USkeletalMeshComponent* FindFirstMeshWithAsset(const AActor* Actor)
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

	const FKey NumPadKeys[] = {
		EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
		EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine
	};
}

AMyPlayerController::AMyPlayerController()
{
	bShowMouseCursor = false;
	bAutoManageActiveCameraTarget = false;
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ConfigureAvatarCapture();
	SetupStage();

	if (!TalentActor)
	{
		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, this, &AMyPlayerController::SetupStage, TalentSearchRetryDelay, false);
	}
}

void AMyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bAvatarCaptureMode && !FApp::HasFocus())
	{
		PollNumPadWhileUnfocused();
	}
}

void AMyPlayerController::ConfigureAvatarCapture()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("AvatarCapture")))
	{
		return;
	}
	bAvatarCaptureMode = true;

	if (UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		UserSettings->SetFullscreenMode(EWindowMode::Windowed);
		UserSettings->SetScreenResolution(CaptureResolution);
		UserSettings->SetFrameRateLimit(FMath::Max(0.f, CaptureFrameRateLimit));
		UserSettings->SetVSyncEnabled(false);
		UserSettings->ApplySettings(false);
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AMyPlayerController::FinalizeAvatarCaptureWindowAfterResize);
}

void AMyPlayerController::PollNumPadWhileUnfocused()
{
#if PLATFORM_WINDOWS
	for (int32 Index = 0; Index < NumPadKeyCount; ++Index)
	{
		const bool bIsDown = (::GetAsyncKeyState(VK_NUMPAD0 + Index) & 0x8000) != 0;
		if (bIsDown && !NumPadKeyWasDown[Index] && ExpressionPresets.IsValidIndex(Index))
		{
			Face(Index);
		}
		NumPadKeyWasDown[Index] = bIsDown;
	}
#endif
}

void AMyPlayerController::FinalizeAvatarCaptureWindowAfterResize()
{
	FIntPoint ViewportSize = FIntPoint::ZeroValue;
	if (GEngine && GEngine->GameViewport)
	{
		if (TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow())
		{
			GameWindow->SetTitle(FText::FromString(TEXT("InBang Avatar Capture")));
		}

		if (GEngine->GameViewport->Viewport)
		{
			ViewportSize = GEngine->GameViewport->Viewport->GetSizeXY();
		}
	}

	UE_LOG(LogInBang, Log, TEXT("AvatarCapture 활성: Windowed, 뷰포트 %dx%d (요청 %dx%d), %.0ffps 제한, 창 제목 'InBang Avatar Capture'"),
		ViewportSize.X, ViewportSize.Y, CaptureResolution.X, CaptureResolution.Y, CaptureFrameRateLimit);
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	const int32 NumBindings = FMath::Min(ExpressionPresets.Num(), NumPadKeyCount);
	for (int32 Index = 0; Index < NumBindings; ++Index)
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

	if (bAutoBlink && !bBlinkActive && TalentActor)
	{
		bBlinkActive = true;
		ScheduleNextBlink();
		UE_LOG(LogInBang, Log, TEXT("자동 눈 깜빡임 활성 (%.1f~%.1f초 간격)"), BlinkIntervalMin, BlinkIntervalMax);
	}
}

void AMyPlayerController::ScheduleNextBlink()
{
	const float Delay = FMath::FRandRange(BlinkIntervalMin, FMath::Max(BlinkIntervalMin, BlinkIntervalMax));
	GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &AMyPlayerController::StartBlink, Delay, false);
}

void AMyPlayerController::StartBlink()
{
	SetBlinkWeight(1.f);
	GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &AMyPlayerController::EndBlink, FMath::Max(0.02f, BlinkClosedDuration), false);
}

void AMyPlayerController::EndBlink()
{
	SetBlinkWeight(0.f);
	if (bBlinkActive)
	{
		ScheduleNextBlink();
	}
}

void AMyPlayerController::SetBlinkWeight(float Weight)
{
	if (USkeletalMeshComponent* Mesh = GetTalentMesh())
	{
		for (const FName& Morph : BlinkMorphs)
		{
			Mesh->SetMorphTarget(Morph, Weight);
		}
	}
}

void AMyPlayerController::AutoBlink(bool bEnable)
{
	bAutoBlink = bEnable;

	if (bEnable && !bBlinkActive && TalentActor)
	{
		bBlinkActive = true;
		ScheduleNextBlink();
	}
	else if (!bEnable)
	{
		bBlinkActive = false;
		GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
		SetBlinkWeight(0.f);
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
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			if (*It != GetPawn() && FindFirstMeshWithAsset(*It))
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
	return FindFirstMeshWithAsset(TalentActor);
}

FVector AMyPlayerController::GetFocusLocation() const
{
	if (const USkeletalMeshComponent* Mesh = GetTalentMesh())
	{
		const FBoxSphereBounds& Bounds = Mesh->Bounds;
		const double FeetZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
		const double Height = 2.0 * Bounds.BoxExtent.Z;
		return FVector(Bounds.Origin.X, Bounds.Origin.Y, FeetZ + Height * CameraFocusHeightRatioFromFeet);
	}

	return TalentActor ? TalentActor->GetActorLocation() : FVector::ZeroVector;
}

void AMyPlayerController::SetupBroadcastCamera()
{
	UWorld* World = GetWorld();

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

	const FVector Focus = GetFocusLocation();
	const FVector CameraLocation = Focus + TalentActor->GetActorForwardVector() * CameraDistance;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(CameraLocation, (Focus - CameraLocation).Rotation(), Params);
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
			return;
		}
	}

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!PlaneMesh)
	{
		return;
	}

	const FVector TalentForward = TalentActor->GetActorForwardVector();
	const FVector Location = GetFocusLocation() - TalentForward * BackdropDistanceBehindTalent;
	const FRotator PlaneNormalFacingCamera = FRotationMatrix::MakeFromZ(TalentForward).Rotator();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Backdrop = World->SpawnActor<AStaticMeshActor>(Location, PlaneNormalFacingCamera, Params);
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

	UMaterialInterface* Material = Cast<UMaterialInterface>(UnlitBackdropMaterial.TryLoad());
	if (!Material)
	{
		if (UMaterialInterface* BasicShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* Tinted = UMaterialInstanceDynamic::Create(BasicShapeMaterial, Backdrop);
			Tinted->SetVectorParameterValue(TEXT("Color"), BackdropColor);
			Material = Tinted;
		}
	}
	if (Material)
	{
		MeshComp->SetMaterial(0, Material);
	}

	UE_LOG(LogInBang, Log, TEXT("그린스크린 배경 자동 생성 (%.0fcm 뒤)"), BackdropDistanceBehindTalent);
}

void AMyPlayerController::Face(int32 PresetIndex)
{
	if (!ExpressionPresets.IsValidIndex(PresetIndex))
	{
		UE_LOG(LogInBang, Warning, TEXT("표정 프리셋 %d 없음 (0~%d)"), PresetIndex, ExpressionPresets.Num() - 1);
		return;
	}

	const FExpressionPreset& Preset = ExpressionPresets[PresetIndex];
	ApplyPreset(Preset);
	UE_LOG(LogInBang, Log, TEXT("표정 프리셋 %d '%s' 적용 (모프 %d개)"), PresetIndex, *Preset.Label.ToString(), TouchedMorphs.Num());
	OnExpressionChanged(PresetIndex);
}

void AMyPlayerController::ResetTouchedMorphs(USkeletalMeshComponent& Mesh)
{
	for (const FName& Touched : TouchedMorphs)
	{
		Mesh.SetMorphTarget(Touched, 0.f);
	}
	TouchedMorphs.Reset();
}

void AMyPlayerController::ApplyPreset(const FExpressionPreset& Preset)
{
	USkeletalMeshComponent* Mesh = GetTalentMesh();
	if (!Mesh)
	{
		UE_LOG(LogInBang, Warning, TEXT("표정 적용 실패: 방송 주인공 메시 없음"));
		return;
	}

	ResetTouchedMorphs(*Mesh);

	const USkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();
	for (const FMorphWeight& Morph : Preset.Morphs)
	{
		if (Asset && !Asset->FindMorphTarget(Morph.Name))
		{
			UE_LOG(LogInBang, Warning, TEXT("모프타겟 '%s'가 %s에 없습니다. 콘솔(~)에서 ListMorphs로 확인하세요."),
				*Morph.Name.ToString(), *Asset->GetName());
			continue;
		}

		Mesh->SetMorphTarget(Morph.Name, Morph.Weight);
		TouchedMorphs.Add(Morph.Name);
	}
}

void AMyPlayerController::RefreshCameraTransform()
{
	if (!BroadcastCamera || !TalentActor)
	{
		return;
	}

	const FVector Focus = GetFocusLocation();
	const FVector CameraLocation = Focus + TalentActor->GetActorForwardVector() * CameraDistance;
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

void AMyPlayerController::Expr(FName MorphTarget, float Weight)
{
	if (USkeletalMeshComponent* Mesh = GetTalentMesh())
	{
		Mesh->SetMorphTarget(MorphTarget, Weight);
		TouchedMorphs.Add(MorphTarget);
	}
}

void AMyPlayerController::CamDist(float Centimeters)
{
	CameraDistance = Centimeters;
	RefreshCameraTransform();
}

void AMyPlayerController::CamHeight(float RatioFromFeetToHead)
{
	CameraFocusHeightRatioFromFeet = RatioFromFeetToHead;
	RefreshCameraTransform();
}

void AMyPlayerController::CamFocal(float Millimeters)
{
	CameraFocalLength = Millimeters;
	if (BroadcastCamera)
	{
		if (UCineCameraComponent* CineComp = BroadcastCamera->GetCineCameraComponent())
		{
			CineComp->SetCurrentFocalLength(CameraFocalLength);
		}
	}
}
