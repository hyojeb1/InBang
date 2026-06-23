#include "MyPlayerController.h"

#include "InputCoreTypes.h"
#include "LeDuatInGameWidget.h"
#include "MyHUD.h"

AMyPlayerController::AMyPlayerController()
{
	bShowMouseCursor = false;
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AMyPlayerController::HandleNeutralExpression);
	InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AMyPlayerController::HandleAltExpression);
	InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AMyPlayerController::LeDuatToggleMission);
	InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AMyPlayerController::LeDuatToggleInventory);
	InputComponent->BindKey(EKeys::J, IE_Pressed, this, &AMyPlayerController::LeDuatToggleCollection);
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AMyPlayerController::LeDuatToggleGod);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AMyPlayerController::LeDuatToggleGodChat);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMyPlayerController::LeDuatToggleEscMenu);
}

void AMyPlayerController::HandleNeutralExpression()
{
	OnNeutralExpressionRequested();
}

void AMyPlayerController::HandleAltExpression()
{
	OnAltExpressionRequested();
}

ULeDuatInGameWidget* AMyPlayerController::GetLeDuatWidget() const
{
	if (const AMyHUD* InBangHUD = Cast<AMyHUD>(GetHUD()))
	{
		return InBangHUD->GetLeDuatWidget();
	}

	return nullptr;
}

void AMyPlayerController::LeDuatToggleMission()
{
	if (ULeDuatInGameWidget* Widget = GetLeDuatWidget())
	{
		Widget->ToggleMissionWindow();
	}
}

void AMyPlayerController::LeDuatToggleInventory()
{
	if (ULeDuatInGameWidget* Widget = GetLeDuatWidget())
	{
		Widget->ToggleInventoryWindow();
	}
}

void AMyPlayerController::LeDuatToggleCollection()
{
	if (ULeDuatInGameWidget* Widget = GetLeDuatWidget())
	{
		Widget->ToggleCollectionWindow();
	}
}

void AMyPlayerController::LeDuatToggleGod()
{
	if (ULeDuatInGameWidget* Widget = GetLeDuatWidget())
	{
		Widget->ToggleGodWindow();
	}
}

void AMyPlayerController::LeDuatToggleGodChat()
{
	if (ULeDuatInGameWidget* Widget = GetLeDuatWidget())
	{
		Widget->ToggleGodChat();
	}
}

void AMyPlayerController::LeDuatToggleEscMenu()
{
	if (ULeDuatInGameWidget* Widget = GetLeDuatWidget())
	{
		Widget->ToggleEscMenu();
	}
}
