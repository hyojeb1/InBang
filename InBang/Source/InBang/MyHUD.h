#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MyHUD.generated.h"

UCLASS()
class INBANG_API AMyHUD : public AHUD
{
	GENERATED_BODY()

public:
	AMyHUD();

	virtual void BeginPlay() override;

	class ULeDuatInGameWidget* GetLeDuatWidget() const { return LeDuatWidget; }

private:
	UPROPERTY()
	TObjectPtr<class ULeDuatInGameWidget> LeDuatWidget;
};
