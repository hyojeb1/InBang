#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CalculatorOperation.h"
#include "CalculatorBlueprintLibrary.generated.h"

UCLASS()
class CALCULATORRUNTIME_API UCalculatorBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Calculator")
	static bool Calculate(
		ECalculatorOperation Operation,
		double A,
		double B,
		double& Result,
		FString& ErrorMessage);
};
