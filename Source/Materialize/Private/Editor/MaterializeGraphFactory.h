#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MaterializeGraphFactory.generated.h"

UCLASS()
class UMaterializeGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMaterializeGraphFactory();

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override { return true; }
	virtual FText GetDisplayName() const override;
	//~ End UFactory Interface
};
