#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class ACineCameraActor;

/**
 * 버튜버 방송용 컨트롤러.
 * BeginPlay에서 알아서 방송 무대를 구성하므로 BP 작업이 필요 없다:
 *  1. 레벨에서 네페르(태그 "Nefer" 또는 첫 번째 캐릭터)를 찾고
 *  2. 카메라가 없으면 버스트샷 CineCamera를 자동 생성하고
 *  3. 그린스크린 배경 판을 자동 생성한다.
 * Numpad 0~5 = ExpressionSlots에 정의된 표정 프리셋 전환.
 * 세부 값은 DefaultGame.ini [/Script/InBang.MyPlayerController]에서 수정.
 */
UCLASS(Config = Game)
class INBANG_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	// 표정 슬롯 적용 (0 = F1). 콘솔/BP 어디서든 호출 가능
	UFUNCTION(Exec, BlueprintCallable, Category = "Broadcast|Expression")
	void Face(int32 SlotIndex);

	// --- ` 콘솔 명령 (방송 중 실시간 튜닝용) ---

	// 네페르 메시의 모든 모프타겟 이름을 화면/로그에 출력
	UFUNCTION(Exec)
	void ListMorphs();

	// 모프타겟 하나를 직접 실험: Expr Eye_Joy_L 1.0
	UFUNCTION(Exec)
	void Expr(FName MorphTarget, float Value);

	// 자동 눈 깜빡임 켜기/끄기: AutoBlink true / AutoBlink false
	UFUNCTION(Exec)
	void AutoBlink(bool bEnable);

	// 카메라 거리(cm): CamDist 200
	UFUNCTION(Exec)
	void CamDist(float Distance);

	// 카메라 높이 비율(0=발끝, 1=정수리): CamHeight 0.75
	UFUNCTION(Exec)
	void CamHeight(float Ratio);

	// 초점거리(mm): CamFocal 35
	UFUNCTION(Exec)
	void CamFocal(float FocalLength);

protected:
	virtual void SetupInputComponent() override;

	// 표정 전환에 추가 연출(사운드, 파티클 등)이 필요할 때만 BP에서 구현하는 선택 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "Broadcast|Expression")
	void OnExpressionChanged(int32 SlotIndex);

	// --- 설정 (DefaultGame.ini에서 수정 가능) ---

	// 방송 주인공을 찾을 액터 태그. 없으면 내 폰이 아닌 첫 스켈레탈 캐릭터 사용
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast")
	FName TalentActorTag = TEXT("Nefer");

	// 표정 프리셋. 배열 순서대로 F1, F2, ... 에 바인딩된다.
	// 형식: "모프이름,모프이름=0.5" (가중치 생략 시 1.0), "None" = 기본 표정
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Expression")
	TArray<FString> ExpressionSlots;

	// 이 태그가 붙은 카메라가 레벨에 있으면 그걸 쓰고, 없으면 자동 생성
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	FName CameraActorTag = TEXT("BroadcastCamera");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	float CameraDistance = 140.f;

	// 카메라가 바라보는 높이. 0=발끝, 1=정수리 (버스트샷 ≈ 0.8)
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	float CameraFocusHeightRatio = 0.8f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Camera")
	float CameraFocalLength = 50.f;

	// 자동 눈 깜빡임 (표정과 무관하게 위에 얹힌다)
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	bool bAutoBlink = true;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	FString BlinkMorphs = TEXT("Eye_Blink_L,Eye_Blink_R");

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	float BlinkIntervalMin = 2.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	float BlinkIntervalMax = 6.f;

	// 눈 감고 있는 시간(초)
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|Blink")
	float BlinkHoldTime = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	bool bSpawnGreenScreenBackdrop = true;

	// 이 태그가 붙은 액터가 이미 있으면 배경을 새로 만들지 않음
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	FName BackdropActorTag = TEXT("GreenScreen");

	// 캐릭터 뒤로 배경 판을 놓을 거리(cm)
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	float BackdropDistance = 300.f;

	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	FLinearColor BackdropColor = FLinearColor(0.f, 1.f, 0.f);

	// 언릿 단색 머티리얼 에셋을 만들면 여기 경로로 자동 사용 (크로마키 품질 ↑).
	// 없으면 엔진 기본 머티리얼 + BackdropColor로 대체
	UPROPERTY(Config, EditAnywhere, Category = "Broadcast|GreenScreen")
	FSoftObjectPath BackdropMaterialPath = FSoftObjectPath(TEXT("/Game/M_GreenScreen.M_GreenScreen"));

private:
	void ConfigureAvatarCapture();
	void FinalizeAvatarCaptureWindow();
	void PollGlobalExpressionKeys();
	void SetupStage();
	void FindTalent();
	void SetupBroadcastCamera();
	void SpawnBackdrop();
	void RefreshCameraTransform();

	class USkeletalMeshComponent* GetTalentMesh() const;
	FVector GetFocusLocation() const;

	// "모프이름=가중치,..." 문자열을 파싱해 적용. 이전 표정은 자동 리셋
	void ApplyExpressionString(const FString& Expression);

	void ScheduleNextBlink();
	void StartBlink();
	void EndBlink();
	void SetBlinkWeight(float Weight);

	UPROPERTY()
	TObjectPtr<AActor> TalentActor;

	UPROPERTY()
	TObjectPtr<ACineCameraActor> BroadcastCamera;

	// 표정 전환 시 리셋해야 하는, 지금까지 건드린 모프들
	TSet<FName> TouchedMorphs;

	FTimerHandle BlinkTimerHandle;

	bool bCameraReady = false;
	bool bBackdropReady = false;
	bool bBlinkActive = false;
	bool bAvatarCaptureMode = false;
	bool GlobalNumPadKeyStates[6] = {};
};
