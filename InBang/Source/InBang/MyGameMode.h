#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyGameMode.generated.h"

/**
 * 방송 무대용 게임모드.
 * 조작할 폰은 필요 없으므로 스폰하지 않고, 컨트롤러가 레벨의 아바타와 카메라를 직접 다룬다.
 */
UCLASS()
class INBANG_API AMyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyGameMode();
};
