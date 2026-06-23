#include "CalculatorEngine.h"

bool FCalculatorEngine::Calculate(
	ECalculatorOperation Operation,
	double A,
	double B,
	double& OutResult,
	FString& OutErrorMessage)
{
	OutErrorMessage.Reset();

	switch (Operation)
	{
	case ECalculatorOperation::Add:
		OutResult = A + B;
		return true;

	case ECalculatorOperation::Subtract:
		OutResult = A - B;
		return true;

	case ECalculatorOperation::Multiply:
		OutResult = A * B;
		return true;

	case ECalculatorOperation::Divide:
		if (FMath::IsNearlyZero(B))
		{
			OutResult = 0.0;
			OutErrorMessage = TEXT("Cannot divide by zero.");
			return false;
		}

		OutResult = A / B;
		return true;

	default:
		OutResult = 0.0;
		OutErrorMessage = TEXT("Unsupported calculator operation.");
		return false;
	}
}
