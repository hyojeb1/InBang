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

protected:
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Expression")
	void OnNeutralExpressionRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Expression")
	void OnAltExpressionRequested();

private:
	void HandleNeutralExpression();
	void HandleAltExpression();
};
