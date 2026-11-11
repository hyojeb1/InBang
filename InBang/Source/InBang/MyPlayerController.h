#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class ACineCameraActor;
class USkeletalMesh;
class USkeletalMeshComponent;
class UTalentFaceAnimInstance;

USTRUCT(BlueprintType)
struct FCurveWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Curve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct FExpressionPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCurveWeight> Curves;
};

struct FCurveReadback
{
	int32 Requested = 0;
	int32 Applied = 0;
	int32 ActiveMorphs = 0;
	double MaxJointDeltaCm = 0.0;
	TArray<FName> Missing;

	bool AllApplied() const { return Applied == Requested && Missing.IsEmpty(); }
};

UCLASS(Config = Game)
class INBANG_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(Exec, BlueprintCallable, Category = "Broadcast|Expression")
	void Face(int32 PresetIndex);

	UFUNCTION(Exec)
	void Expr(FName Curve, float Weight);

	UFUNCTION(Exec)
	void DumpFaceCurves();

	UFUNCTION(Exec)
	void DumpMeshCurves(const FString& SkeletalMeshPath);

	UFUNCTION(Exec)
	void AutoBlink(bool bEnable);

	UFUNCTION(Exec)
	void CamDist(float Centimeters);

	UFUNCTION(Exec)
	void CamHeight(float CentimetersFromFocusBone);

	UFUNCTION(Exec)
	void CamFocal(float Millimeters);

	UFUNCTION(Exec)
	void VerifyStage();

protected:
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Broadcast|Expression")
	void OnExpressionChanged(int32 PresetIndex);

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast")
	FName TalentActorTag = TEXT("Talent");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Face")
	FName FaceComponentName = TEXT("Face");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Face")
	FName BodyComponentName = TEXT("Body");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Face")
	TSoftClassPtr<UTalentFaceAnimInstance> FaceAnimClass;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Expression")
	TArray<FExpressionPreset> ExpressionPresets;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	FName CameraActorTag = TEXT("BroadcastCamera");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (Units = "Centimeters", ClampMin = "0"))
	float CameraDistance = 180.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	FName CameraFocusBone = TEXT("head");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (Units = "Centimeters"))
	float CameraFocusOffsetAboveBone = 0.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (Units = "Degrees", ClampMin = "0", ClampMax = "90"))
	float VerifyMaxCameraAngleFromTalentFront = 20.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (Units = "Millimeters", ClampMin = "1"))
	float CameraFocalLength = 50.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	bool bAutoBlink = true;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	TArray<FName> BlinkCurves;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink", meta = (Units = "Seconds", ClampMin = "0"))
	float BlinkIntervalMin = 2.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink", meta = (Units = "Seconds", ClampMin = "0"))
	float BlinkIntervalMax = 6.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink", meta = (Units = "Seconds", ClampMin = "0.02"))
	float BlinkClosedDuration = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Capture")
	FIntPoint CaptureResolution = FIntPoint(1920, 1080);

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Capture", meta = (ClampMin = "0"))
	float CaptureFrameRateLimit = 60.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	bool bSpawnGreenScreenBackdrop = false;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	FName BackdropActorTag = TEXT("GreenScreen");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen", meta = (Units = "Centimeters", ClampMin = "0"))
	float BackdropDistanceBehindTalent = 300.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	FLinearColor BackdropColor = FLinearColor(0.f, 1.f, 0.f);

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	FSoftObjectPath UnlitBackdropMaterial = FSoftObjectPath(TEXT("/Game/M_GreenScreen.M_GreenScreen"));

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Verify", meta = (Units = "Seconds", ClampMin = "0.05"))
	float VerifyStepInterval = 0.3f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Verify", meta = (Units = "Seconds", ClampMin = "0"))
	float VerifyStartDelay = 1.f;

private:
	static constexpr int32 NumPadKeyCount = 10;
	static constexpr float TalentSearchRetryDelay = 0.5f;
	static constexpr float CurveReadbackTolerance = 0.01f;
	static constexpr double DeformedJointDeltaCm = 0.05;

	void ConfigureAvatarCapture();
	void FinalizeAvatarCaptureWindowAfterResize();
	void PollNumPadWhileUnfocused();
	void SetupStage();
	void FindTalent();
	void BindTalentMeshes();
	void SetupBroadcastCamera();
	void SpawnBackdrop();
	void RefreshCameraTransform();

	FVector GetFocusLocation() const;
	FVector GetTalentFacingDirection() const;
	double GetCameraAngleFromTalentFront() const;
	bool IsWorldPointInCameraFrame(const FVector& WorldPoint) const;
	bool IsFaceInCameraFrame() const;

	void PushFaceCurves();
	FCurveReadback ReadBackFaceCurves(const TMap<FName, float>& Expected) const;

	void ScheduleNextBlink();
	void StartBlink();
	void EndBlink();
	void SetBlinkWeight(float Weight);
	void StopAutoBlink();

	static TArray<FName> ReadRigControlNames(USkeletalMesh* Mesh);
	static void LogMeshCurves(const FString& Source, USkeletalMesh* Mesh);

	void BuildVerifySteps();
	void RunNextVerifyStep();
	void RecordVerify(bool bPassed, const FString& Line);

	UPROPERTY()
	TObjectPtr<AActor> TalentActor;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> FaceMesh;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY()
	TObjectPtr<ACineCameraActor> BroadcastCamera;

	TSet<FName> RigControlNames;
	TMap<FName, float> PresetCurves;
	TMap<FName, float> ManualCurves;
	float BlinkWeight = 0.f;

	FTimerHandle BlinkTimerHandle;
	FTimerHandle VerifyTimerHandle;
	TArray<TFunction<void()>> VerifySteps;
	TArray<FVector> NeutralJointLocations;
	int32 NeutralActiveMorphs = 0;
	int32 NextVerifyStep = 0;
	int32 VerifyFailures = 0;
	bool bQuitAfterVerify = false;

	bool bCameraReady = false;
	bool bBackdropReady = false;
	bool bBlinkActive = false;
	bool bAvatarCaptureMode = false;
	bool NumPadKeyWasDown[NumPadKeyCount] = {};
};
