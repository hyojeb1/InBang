#pragma once

#include "Modules/ModuleManager.h"

class FCalculatorCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
