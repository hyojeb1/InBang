#include "MyPlayerController.h"

#include "Animation/AnimInstance.h"
#include "Animation/Skeleton.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DNAReader.h"
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
#include "SkelMeshDNAUtils.h"
#include "TalentFaceAnimInstance.h"
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

	FString GetNameOrNone(const UObject* Object)
	{
		return Object ? Object->GetName() : TEXT("none");
	}

	FString JoinNames(const TArray<FName>& Names)
	{
		return FString::JoinBy(Names, TEXT(","), [](const FName& Name) { return Name.ToString(); });
	}

	TMap<FName, float> ToCurveMap(const TArray<FCurveWeight>& Curves)
	{
		TMap<FName, float> Map;
		for (const FCurveWeight& Curve : Curves)
		{
			Map.Add(Curve.Curve, Curve.Weight);
		}
		return Map;
	}

	const FKey NumPadKeys[] = {
		EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
		EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine
	};

	constexpr float ActiveMorphThreshold = 1e-3f;
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

	if (FParse::Param(FCommandLine::Get(), TEXT("InBangVerify")))
	{
		bQuitAfterVerify = true;
		FTimerHandle VerifyStartHandle;
		GetWorldTimerManager().SetTimer(VerifyStartHandle, this, &AMyPlayerController::VerifyStage, FMath::Max(VerifyStartDelay, TalentSearchRetryDelay + 0.1f), false);
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
		BindTalentMeshes();
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

	if (bAutoBlink && !bBlinkActive && FaceMesh)
	{
		bBlinkActive = true;
		ScheduleNextBlink();
		UE_LOG(LogInBang, Log, TEXT("자동 눈 깜빡임 활성 (%.1f~%.1f초 간격)"), BlinkIntervalMin, BlinkIntervalMax);
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

void AMyPlayerController::BindTalentMeshes()
{
	if (!TalentActor)
	{
		return;
	}

	TInlineComponentArray<USkeletalMeshComponent*> Meshes(TalentActor.Get());
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetFName() == FaceComponentName)
		{
			FaceMesh = Mesh;
		}
		else if (Mesh->GetFName() == BodyComponentName)
		{
			BodyMesh = Mesh;
		}
	}

	if (!BodyMesh && FaceMesh)
	{
		BodyMesh = Cast<USkeletalMeshComponent>(FaceMesh->GetAttachParent());
	}
	if (!BodyMesh)
	{
		BodyMesh = FindFirstMeshWithAsset(TalentActor);
	}

	if (!FaceMesh)
	{
		UE_LOG(LogInBang, Warning, TEXT("얼굴 컴포넌트 '%s'가 %s에 없습니다. DumpFaceCurves로 컴포넌트 이름을 확인하고 FaceComponentName을 맞춰주세요."),
			*FaceComponentName.ToString(), *TalentActor->GetName());
		return;
	}

	for (USkeletalMeshComponent* Mesh : { FaceMesh.Get(), BodyMesh.Get() })
	{
		if (Mesh)
		{
			Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		}
	}

	RigControlNames = TSet<FName>(ReadRigControlNames(FaceMesh->GetSkeletalMeshAsset()));

	if (UClass* AnimClass = FaceAnimClass.LoadSynchronous())
	{
		FaceMesh->SetAnimInstanceClass(AnimClass);
	}
	else
	{
		UE_LOG(LogInBang, Warning, TEXT("FaceAnimClass '%s'를 불러오지 못했습니다. 표정·깜빡임이 얼굴에 전달되지 않습니다."), *FaceAnimClass.ToString());
	}

	UE_LOG(LogInBang, Log, TEXT("얼굴 바인딩: face=%s mesh=%s skeleton=%s anim=%s postProcess=%s rigControls=%d body=%s"),
		*FaceMesh->GetName(),
		*GetNameOrNone(FaceMesh->GetSkeletalMeshAsset()),
		*(FaceMesh->GetSkeletalMeshAsset() && FaceMesh->GetSkeletalMeshAsset()->GetSkeleton() ? FaceMesh->GetSkeletalMeshAsset()->GetSkeleton()->GetPathName() : FString(TEXT("none"))),
		*GetNameOrNone(FaceMesh->GetAnimInstance() ? FaceMesh->GetAnimInstance()->GetClass() : nullptr),
		*GetNameOrNone(FaceMesh->GetPostProcessInstance() ? FaceMesh->GetPostProcessInstance()->GetClass() : nullptr),
		RigControlNames.Num(),
		*GetNameOrNone(BodyMesh));
}

FVector AMyPlayerController::GetFocusLocation() const
{
	const FVector Up = FVector::UpVector * CameraFocusOffsetAboveBone;
	if (FaceMesh && FaceMesh->DoesSocketExist(CameraFocusBone))
	{
		return FaceMesh->GetSocketLocation(CameraFocusBone) + Up;
	}
	if (FaceMesh)
	{
		return FaceMesh->Bounds.Origin + Up;
	}
	return TalentActor ? TalentActor->GetActorLocation() : FVector::ZeroVector;
}

FVector AMyPlayerController::GetTalentFacingDirection() const
{
	if (const USkeletalMeshComponent* FacingSource = BodyMesh ? BodyMesh : FaceMesh)
	{
		const FVector MetaHumanMeshForwardIsComponentPlusY = FacingSource->GetRightVector();
		const FVector Horizontal = FVector(MetaHumanMeshForwardIsComponentPlusY.X, MetaHumanMeshForwardIsComponentPlusY.Y, 0.0).GetSafeNormal();
		if (!Horizontal.IsZero())
		{
			return Horizontal;
		}
	}
	return TalentActor ? TalentActor->GetActorForwardVector() : FVector::ForwardVector;
}

double AMyPlayerController::GetCameraAngleFromTalentFront() const
{
	if (!BroadcastCamera)
	{
		return 180.0;
	}
	const FVector ToCamera = (BroadcastCamera->GetActorLocation() - GetFocusLocation()).GetSafeNormal2D();
	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToCamera, GetTalentFacingDirection()), -1.0, 1.0)));
}

bool AMyPlayerController::IsWorldPointInCameraFrame(const FVector& WorldPoint) const
{
	const UCineCameraComponent* CineComp = BroadcastCamera ? BroadcastCamera->GetCineCameraComponent() : nullptr;
	if (!CineComp)
	{
		return false;
	}

	const FVector Local = BroadcastCamera->GetActorTransform().InverseTransformPosition(WorldPoint);
	if (Local.X <= 0.0)
	{
		return false;
	}

	const double HorizontalAngle = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Local.Y), Local.X));
	const double VerticalAngle = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Local.Z), Local.X));
	return HorizontalAngle < CineComp->GetHorizontalFieldOfView() * 0.5 && VerticalAngle < CineComp->GetVerticalFieldOfView() * 0.5;
}

bool AMyPlayerController::IsFaceInCameraFrame() const
{
	if (!FaceMesh)
	{
		return false;
	}
	const FBoxSphereBounds& Bounds = FaceMesh->Bounds;
	const FVector HeadTop = Bounds.Origin + FVector(0.0, 0.0, Bounds.BoxExtent.Z);
	return IsWorldPointInCameraFrame(GetFocusLocation()) && IsWorldPointInCameraFrame(HeadTop);
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
	const FVector CameraLocation = Focus + GetTalentFacingDirection() * CameraDistance;

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

	const FVector TalentForward = GetTalentFacingDirection();
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

void AMyPlayerController::RefreshCameraTransform()
{
	if (!BroadcastCamera || !TalentActor)
	{
		return;
	}

	const FVector Focus = GetFocusLocation();
	const FVector CameraLocation = Focus + GetTalentFacingDirection() * CameraDistance;
	BroadcastCamera->SetActorLocationAndRotation(CameraLocation, (Focus - CameraLocation).Rotation());
}

void AMyPlayerController::PushFaceCurves()
{
	UTalentFaceAnimInstance* FaceAnim = FaceMesh ? Cast<UTalentFaceAnimInstance>(FaceMesh->GetAnimInstance()) : nullptr;
	if (!FaceAnim)
	{
		return;
	}

	TMap<FName, float> Curves = PresetCurves;
	Curves.Append(ManualCurves);
	if (BlinkWeight > 0.f)
	{
		for (const FName& Blink : BlinkCurves)
		{
			float& Value = Curves.FindOrAdd(Blink);
			Value = FMath::Max(Value, BlinkWeight);
		}
	}
	FaceAnim->SetCurvesWrittenAfterGraph(MoveTemp(Curves));
}

FCurveReadback AMyPlayerController::ReadBackFaceCurves(const TMap<FName, float>& Expected) const
{
	FCurveReadback Result;
	Result.Requested = Expected.Num();

	const UAnimInstance* FaceAnim = FaceMesh ? FaceMesh->GetAnimInstance() : nullptr;
	for (const TPair<FName, float>& Curve : Expected)
	{
		float Evaluated = 0.f;
		const bool bEvaluated = FaceAnim && FaceAnim->GetCurveValue(Curve.Key, Evaluated);
		const bool bRigConsumes = RigControlNames.Contains(Curve.Key);
		if (bEvaluated && bRigConsumes && FMath::IsNearlyEqual(Evaluated, Curve.Value, CurveReadbackTolerance))
		{
			++Result.Applied;
		}
		else
		{
			Result.Missing.Add(Curve.Key);
		}
	}

	if (!FaceMesh)
	{
		return Result;
	}

	for (const TPair<FName, float>& Morph : FaceMesh->GetMorphTargetCurves())
	{
		if (FMath::Abs(Morph.Value) > ActiveMorphThreshold)
		{
			++Result.ActiveMorphs;
		}
	}

	const TArray<FTransform>& Joints = FaceMesh->GetComponentSpaceTransforms();
	if (Joints.Num() == NeutralJointLocations.Num())
	{
		for (int32 Index = 0; Index < Joints.Num(); ++Index)
		{
			Result.MaxJointDeltaCm = FMath::Max(Result.MaxJointDeltaCm, FVector::Dist(Joints[Index].GetLocation(), NeutralJointLocations[Index]));
		}
	}
	return Result;
}

void AMyPlayerController::Face(int32 PresetIndex)
{
	if (!ExpressionPresets.IsValidIndex(PresetIndex))
	{
		UE_LOG(LogInBang, Warning, TEXT("표정 프리셋 %d 없음 (0~%d)"), PresetIndex, ExpressionPresets.Num() - 1);
		return;
	}

	const FExpressionPreset& Preset = ExpressionPresets[PresetIndex];
	PresetCurves = ToCurveMap(Preset.Curves);
	ManualCurves.Reset();
	PushFaceCurves();

	for (const FCurveWeight& Curve : Preset.Curves)
	{
		if (!RigControlNames.Contains(Curve.Curve))
		{
			UE_LOG(LogInBang, Warning, TEXT("'%s'는 얼굴 RigLogic 컨트롤이 아닙니다. 콘솔에서 DumpFaceCurves로 확인하세요."), *Curve.Curve.ToString());
		}
	}

	UE_LOG(LogInBang, Log, TEXT("표정 프리셋 %d '%s' 적용 (커브 %d개)"), PresetIndex, *Preset.Label.ToString(), PresetCurves.Num());
	OnExpressionChanged(PresetIndex);
}

void AMyPlayerController::Expr(FName Curve, float Weight)
{
	ManualCurves.Add(Curve, Weight);
	PushFaceCurves();
}

TArray<FName> AMyPlayerController::ReadRigControlNames(USkeletalMesh* Mesh)
{
	TArray<FName> Names;
	const TSharedPtr<IDNAReader> Reader = Mesh ? USkelMeshDNAUtils::GetDNAReader(Mesh) : nullptr;
	if (!Reader)
	{
		return Names;
	}

	const uint16 ControlCount = Reader->GetRawControlCount();
	Names.Reserve(ControlCount);
	for (uint16 Index = 0; Index < ControlCount; ++Index)
	{
		FString ObjectName;
		FString AttributeName;
		const FString DNAControlName = Reader->GetRawControlName(Index);
		Names.Add(DNAControlName.Split(TEXT("."), &ObjectName, &AttributeName)
			? FName(ObjectName + TEXT("_") + AttributeName)
			: FName(DNAControlName));
	}
	return Names;
}

void AMyPlayerController::LogMeshCurves(const FString& Source, USkeletalMesh* Mesh)
{
	if (!Mesh)
	{
		UE_LOG(LogInBang, Warning, TEXT("CURVES source=%s mesh=none"), *Source);
		return;
	}

	TArray<FName> SkeletonCurves;
	if (const USkeleton* Skeleton = Mesh->GetSkeleton())
	{
		Skeleton->GetCurveMetaDataNames(SkeletonCurves);
	}

	const TArray<FName> RigControls = ReadRigControlNames(Mesh);
	UE_LOG(LogInBang, Log, TEXT("CURVES source=%s mesh=%s rigControls=%d skeletonCurves=%d morphTargets=%d"),
		*Source, *Mesh->GetName(), RigControls.Num(), SkeletonCurves.Num(), Mesh->GetMorphTargets().Num());

	for (const FName& Control : RigControls)
	{
		UE_LOG(LogInBang, Log, TEXT("CURVES source=%s rigControl=%s"), *Source, *Control.ToString());
	}
}

void AMyPlayerController::DumpFaceCurves()
{
	if (!TalentActor)
	{
		UE_LOG(LogInBang, Warning, TEXT("CURVES talent=none"));
		return;
	}

	TInlineComponentArray<USkeletalMeshComponent*> Meshes(TalentActor.Get());
	for (const USkeletalMeshComponent* Mesh : Meshes)
	{
		const UAnimInstance* Anim = Mesh->GetAnimInstance();
		const UAnimInstance* PostProcess = Mesh->GetPostProcessInstance();
		UE_LOG(LogInBang, Log, TEXT("CURVES component=%s mesh=%s anim=%s postProcess=%s role=%s"),
			*Mesh->GetName(),
			*GetNameOrNone(Mesh->GetSkeletalMeshAsset()),
			*GetNameOrNone(Anim ? Anim->GetClass() : nullptr),
			*GetNameOrNone(PostProcess ? PostProcess->GetClass() : nullptr),
			Mesh == FaceMesh ? TEXT("face") : Mesh == BodyMesh ? TEXT("body") : TEXT("-"));
	}

	if (FaceMesh)
	{
		LogMeshCurves(FaceMesh->GetName(), FaceMesh->GetSkeletalMeshAsset());
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
			FString::Printf(TEXT("DumpFaceCurves: RigLogic 컨트롤 %d개 (로그의 CURVES 줄 참고)"), RigControlNames.Num()));
	}
}

void AMyPlayerController::DumpMeshCurves(const FString& SkeletalMeshPath)
{
	LogMeshCurves(SkeletalMeshPath, LoadObject<USkeletalMesh>(nullptr, *SkeletalMeshPath));
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
	BlinkWeight = Weight;
	PushFaceCurves();
}

void AMyPlayerController::StopAutoBlink()
{
	bBlinkActive = false;
	GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
	SetBlinkWeight(0.f);
}

void AMyPlayerController::AutoBlink(bool bEnable)
{
	bAutoBlink = bEnable;

	if (bEnable && !bBlinkActive && FaceMesh)
	{
		bBlinkActive = true;
		ScheduleNextBlink();
	}
	else if (!bEnable)
	{
		StopAutoBlink();
	}
}

void AMyPlayerController::CamDist(float Centimeters)
{
	CameraDistance = Centimeters;
	RefreshCameraTransform();
}

void AMyPlayerController::CamHeight(float CentimetersFromFocusBone)
{
	CameraFocusOffsetAboveBone = CentimetersFromFocusBone;
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

void AMyPlayerController::VerifyStage()
{
	VerifyFailures = 0;
	NextVerifyStep = 0;
	BuildVerifySteps();
	RunNextVerifyStep();
}

void AMyPlayerController::RecordVerify(bool bPassed, const FString& Line)
{
	if (!bPassed)
	{
		++VerifyFailures;
	}
	UE_LOG(LogInBang, Log, TEXT("VERIFY %s status=%s"), *Line, bPassed ? TEXT("ok") : TEXT("FAIL"));
}

void AMyPlayerController::BuildVerifySteps()
{
	VerifySteps.Reset();

	VerifySteps.Add([this]()
		{
			const UAnimInstance* FaceAnim = FaceMesh ? FaceMesh->GetAnimInstance() : nullptr;
			const bool bDriverReady = Cast<UTalentFaceAnimInstance>(FaceAnim) != nullptr;
			RecordVerify(TalentActor && FaceMesh && bDriverReady && !RigControlNames.IsEmpty(),
				FString::Printf(TEXT("bind talent=%s face=%s faceMesh=%s faceAnim=%s postProcess=%s rigControls=%d body=%s"),
					*GetNameOrNone(TalentActor),
					*GetNameOrNone(FaceMesh),
					*GetNameOrNone(FaceMesh ? FaceMesh->GetSkeletalMeshAsset() : nullptr),
					*GetNameOrNone(FaceAnim ? FaceAnim->GetClass() : nullptr),
					*GetNameOrNone(FaceMesh && FaceMesh->GetPostProcessInstance() ? FaceMesh->GetPostProcessInstance()->GetClass() : nullptr),
					RigControlNames.Num(),
					*GetNameOrNone(BodyMesh)));

			if (!FaceMesh)
			{
				UE_LOG(LogInBang, Log, TEXT("VERIFY result=FAIL failures=%d"), VerifyFailures);
				VerifySteps.SetNum(NextVerifyStep);
				return;
			}
			StopAutoBlink();
		});

	VerifySteps.Add([this]()
		{
			PresetCurves.Reset();
			ManualCurves.Reset();
			PushFaceCurves();
		});
	VerifySteps.Add([this]()
		{
			NeutralJointLocations.Reset();
			for (const FTransform& Joint : FaceMesh->GetComponentSpaceTransforms())
			{
				NeutralJointLocations.Add(Joint.GetLocation());
			}
			NeutralActiveMorphs = ReadBackFaceCurves({}).ActiveMorphs;
			UE_LOG(LogInBang, Log, TEXT("VERIFY neutral joints=%d activeMorphs=%d"), NeutralJointLocations.Num(), NeutralActiveMorphs);
		});

	auto RecordExpression = [this](const FString& Label, const TMap<FName, float>& Expected)
		{
			const FCurveReadback Readback = ReadBackFaceCurves(Expected);
			const bool bDeformed = Readback.ActiveMorphs > NeutralActiveMorphs || Readback.MaxJointDeltaCm > DeformedJointDeltaCm;
			RecordVerify(Readback.AllApplied() && (Readback.Requested == 0 || bDeformed),
				FString::Printf(TEXT("%s requested=%d applied=%d missing=%s activeMorphs=%d maxJointDeltaCm=%.2f"),
					*Label, Readback.Requested, Readback.Applied, *JoinNames(Readback.Missing),
					Readback.ActiveMorphs, Readback.MaxJointDeltaCm));
		};

	for (int32 Index = 0; Index < ExpressionPresets.Num(); ++Index)
	{
		VerifySteps.Add([this, Index]() { Face(Index); });
		VerifySteps.Add([this, Index, RecordExpression]()
			{
				const FExpressionPreset& Preset = ExpressionPresets[Index];
				RecordExpression(TEXT("preset=") + Preset.Label.ToString(), ToCurveMap(Preset.Curves));
			});
	}

	VerifySteps.Add([this]()
		{
			PresetCurves.Reset();
			ManualCurves.Reset();
			SetBlinkWeight(1.f);
		});
	VerifySteps.Add([this, RecordExpression]()
		{
			TMap<FName, float> Closed;
			for (const FName& Blink : BlinkCurves)
			{
				Closed.Add(Blink, 1.f);
			}
			RecordExpression(TEXT("blink"), Closed);
			SetBlinkWeight(0.f);
		});

	VerifySteps.Add([this]()
		{
			const AActor* ViewTarget = GetViewTarget();
			const UCineCameraComponent* CineComp = BroadcastCamera ? BroadcastCamera->GetCineCameraComponent() : nullptr;
			const double Distance = BroadcastCamera ? FVector::Dist(BroadcastCamera->GetActorLocation(), GetFocusLocation()) : 0.0;
			const bool bIsViewTarget = BroadcastCamera && ViewTarget == BroadcastCamera;
			const bool bFaceInFrame = IsFaceInCameraFrame();
			const double AngleFromFront = GetCameraAngleFromTalentFront();
			const bool bFrontal = AngleFromFront <= VerifyMaxCameraAngleFromTalentFront;
			RecordVerify(bIsViewTarget && bFaceInFrame && bFrontal,
				FString::Printf(TEXT("camera=%s viewTarget=%s faceInFrame=%s angleFromFrontDeg=%.0f focusBone=%s faceBoundsTopAboveFocusCm=%.1f faceBoundsHalfHeightCm=%.1f verticalFovDeg=%.1f distance=%.0f focal=%.0f"),
					*GetNameOrNone(BroadcastCamera), *GetNameOrNone(ViewTarget), bFaceInFrame ? TEXT("true") : TEXT("false"),
					AngleFromFront, FaceMesh && FaceMesh->DoesSocketExist(CameraFocusBone) ? *CameraFocusBone.ToString() : TEXT("none"),
					FaceMesh ? FaceMesh->Bounds.Origin.Z + FaceMesh->Bounds.BoxExtent.Z - GetFocusLocation().Z : 0.0,
					FaceMesh ? FaceMesh->Bounds.BoxExtent.Z : 0.0,
					CineComp ? CineComp->GetVerticalFieldOfView() : 0.f,
					Distance, CineComp ? CineComp->CurrentFocalLength : 0.f));
		});

	VerifySteps.Add([this]()
		{
			if (bAutoBlink)
			{
				AutoBlink(true);
			}
			UE_LOG(LogInBang, Log, TEXT("VERIFY result=%s failures=%d"), VerifyFailures == 0 ? TEXT("PASS") : TEXT("FAIL"), VerifyFailures);
		});
}

void AMyPlayerController::RunNextVerifyStep()
{
	if (!VerifySteps.IsValidIndex(NextVerifyStep))
	{
		if (bQuitAfterVerify)
		{
			ConsoleCommand(TEXT("quit"));
		}
		return;
	}

	const int32 StepIndex = NextVerifyStep++;
	VerifySteps[StepIndex]();
	GetWorldTimerManager().SetTimer(VerifyTimerHandle, this, &AMyPlayerController::RunNextVerifyStep, VerifyStepInterval, false);
}
