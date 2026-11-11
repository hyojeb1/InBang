#include "MyGameMode.h"

#include "MyPlayerController.h"

AMyGameMode::AMyGameMode()
{
	PlayerControllerClass = AMyPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
