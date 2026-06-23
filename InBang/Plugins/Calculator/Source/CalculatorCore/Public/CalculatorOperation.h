#pragma once

#include "CoreMinimal.h"
#include "CalculatorOperation.generated.h"

UENUM(BlueprintType)
enum class ECalculatorOperation : uint8
{
	Add UMETA(DisplayName = "Add"),
	Subtract UMETA(DisplayName = "Subtract"),
	Multiply UMETA(DisplayName = "Multiply"),
	Divide UMETA(DisplayName = "Divide")
};
