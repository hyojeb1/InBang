#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

UCLASS()
class INBANG_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();

	UFUNCTION(Exec)
	void LeDuatToggleMission();

	UFUNCTION(Exec)
	void LeDuatToggleInventory();

	UFUNCTION(Exec)
	void LeDuatToggleCollection();

	UFUNCTION(Exec)
	void LeDuatToggleGod();

	UFUNCTION(Exec)
	void LeDuatToggleGodChat();

	UFUNCTION(Exec)
	void LeDuatToggleEscMenu();

protected:
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Expression")
	void OnNeutralExpressionRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Expression")
	void OnAltExpressionRequested();

private:
	class ULeDuatInGameWidget* GetLeDuatWidget() const;

	void HandleNeutralExpression();
	void HandleAltExpression();
};
