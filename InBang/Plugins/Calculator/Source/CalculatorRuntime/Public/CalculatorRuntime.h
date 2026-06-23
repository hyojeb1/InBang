#pragma once

#include "Modules/ModuleManager.h"

class FCalculatorRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
