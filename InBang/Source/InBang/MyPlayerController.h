#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class ACineCameraActor;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FMorphWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

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
	TArray<FMorphWeight> Morphs;
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
	void ListMorphs();

	UFUNCTION(Exec)
	void Expr(FName MorphTarget, float Weight);

	UFUNCTION(Exec)
	void AutoBlink(bool bEnable);

	UFUNCTION(Exec)
	void CamDist(float Centimeters);

	UFUNCTION(Exec)
	void CamHeight(float RatioFromFeetToHead);

	UFUNCTION(Exec)
	void CamFocal(float Millimeters);

protected:
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Broadcast|Expression")
	void OnExpressionChanged(int32 PresetIndex);

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast")
	FName TalentActorTag = TEXT("Talent");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Expression")
	TArray<FExpressionPreset> ExpressionPresets;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	FName CameraActorTag = TEXT("BroadcastCamera");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (Units = "Centimeters", ClampMin = "0"))
	float CameraDistance = 140.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float CameraFocusHeightRatioFromFeet = 0.8f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera", meta = (Units = "Millimeters", ClampMin = "1"))
	float CameraFocalLength = 50.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	bool bAutoBlink = true;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	TArray<FName> BlinkMorphs = { TEXT("eyeBlinkLeft"), TEXT("eyeBlinkRight") };

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

private:
	static constexpr int32 NumPadKeyCount = 10;
	static constexpr float TalentSearchRetryDelay = 0.5f;

	void ConfigureAvatarCapture();
	void FinalizeAvatarCaptureWindowAfterResize();
	void PollNumPadWhileUnfocused();
	void SetupStage();
	void FindTalent();
	void SetupBroadcastCamera();
	void SpawnBackdrop();
	void RefreshCameraTransform();

	USkeletalMeshComponent* GetTalentMesh() const;
	FVector GetFocusLocation() const;

	void ApplyPreset(const FExpressionPreset& Preset);
	void ResetTouchedMorphs(USkeletalMeshComponent& Mesh);

	void ScheduleNextBlink();
	void StartBlink();
	void EndBlink();
	void SetBlinkWeight(float Weight);

	UPROPERTY()
	TObjectPtr<AActor> TalentActor;

	UPROPERTY()
	TObjectPtr<ACineCameraActor> BroadcastCamera;

	TSet<FName> TouchedMorphs;

	FTimerHandle BlinkTimerHandle;

	bool bCameraReady = false;
	bool bBackdropReady = false;
	bool bBlinkActive = false;
	bool bAvatarCaptureMode = false;
	bool NumPadKeyWasDown[NumPadKeyCount] = {};
};
