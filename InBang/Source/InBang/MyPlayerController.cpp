#include "MyPlayerController.h"

#include "InputCoreTypes.h"

AMyPlayerController::AMyPlayerController()
{
	bShowMouseCursor = false;
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AMyPlayerController::HandleNeutralExpression);
	InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AMyPlayerController::HandleAltExpression);
}

void AMyPlayerController::HandleNeutralExpression()
{
	OnNeutralExpressionRequested();
}

void AMyPlayerController::HandleAltExpression()
{
	OnAltExpressionRequested();
}
