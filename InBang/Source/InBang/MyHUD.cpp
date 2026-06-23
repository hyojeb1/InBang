#include "MyHUD.h"

#include "LeDuatInGameWidget.h"
#include "Blueprint/UserWidget.h"

AMyHUD::AMyHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMyHUD::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[장효제] AMyHUD::BeginPlay"));

	APlayerController* OwningPlayer = GetOwningPlayerController();
	if (!OwningPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("[장효제] AMyHUD: OwningPlayer is null"));
		return;
	}

	LeDuatWidget = CreateWidget<ULeDuatInGameWidget>(OwningPlayer, ULeDuatInGameWidget::StaticClass());
	if (!LeDuatWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[장효제] AMyHUD: Failed to create LeDuatWidget"));
		return;
	}

	LeDuatWidget->AddToViewport(10);
	UE_LOG(LogTemp, Warning, TEXT("[장효제] AMyHUD: LeDuatWidget added to viewport"));
}
