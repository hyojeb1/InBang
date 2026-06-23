#include "MyGameMode.h"

#include "MyCharacter.h"
#include "MyHUD.h"
#include "MyPlayerController.h"

AMyGameMode::AMyGameMode()
{
	PlayerControllerClass = AMyPlayerController::StaticClass();
	DefaultPawnClass = AMyCharacter::StaticClass();
	HUDClass = AMyHUD::StaticClass();
}
