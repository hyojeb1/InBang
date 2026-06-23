#pragma once

#include "CoreMinimal.h"
#include "CalculatorOperation.h"

class CALCULATORCORE_API FCalculatorEngine
{
public:
	static bool Calculate(
		ECalculatorOperation Operation,
		double A,
		double B,
		double& OutResult,
		FString& OutErrorMessage);
};
