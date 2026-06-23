#pragma once

#include "Modules/ModuleManager.h"

class SDockTab;

class FCalculatorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OpenCalculatorTab();
	TSharedRef<SDockTab> SpawnCalculatorTab(const FSpawnTabArgs& SpawnTabArgs);
};
