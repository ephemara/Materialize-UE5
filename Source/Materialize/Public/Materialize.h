#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMaterializeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterAssetActions();
	void UnregisterAssetActions();
	void RegisterMenuExtensions();
	void RegisterToolbarExtension();
	void UnregisterToolbarExtension();
};
