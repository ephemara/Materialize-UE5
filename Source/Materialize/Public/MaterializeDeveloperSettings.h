#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MaterializeDeveloperSettings.generated.h"

UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="Materialize"))
class MATERIALIZE_API UMaterializeDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category="Feedback")
	FString BugReportURL = TEXT("");

	UPROPERTY(Config, EditAnywhere, Category="Feedback")
	FString FeatureRequestURL = TEXT("");

	UPROPERTY(Config, EditAnywhere, Category="Feedback")
	FString DiscordURL = TEXT("");

	UPROPERTY(Config, EditAnywhere, Category="Feedback")
	FString DocsURL = TEXT("");
};

