#include "CalculatorBlueprintLibrary.h"

#include "CalculatorEngine.h"

bool UCalculatorBlueprintLibrary::Calculate(
	ECalculatorOperation Operation,
	double A,
	double B,
	double& Result,
	FString& ErrorMessage)
{
	return FCalculatorEngine::Calculate(Operation, A, B, Result, ErrorMessage);
}
