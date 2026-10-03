#include "MaterializeStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyleMacros.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

// Static member initialization
TSharedPtr<FMaterializeStyle> FMaterializeStyle::StyleInstance = nullptr;

// Undefine IMAGE_BRUSH from SlateStyleMacros.h to avoid redefinition warning
#undef IMAGE_BRUSH
#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

FMaterializeStyle::FMaterializeStyle()
	: FSlateStyleSet("MaterializeStyle")
{
	// Set content root to the plugin's Content directory
	const FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("Materialize"))->GetBaseDir() / TEXT("Content");
	SetContentRoot(ContentDir);
	
	// Icon sizes
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	
	// Register toolbar icons
	Set("Materialize.ToolbarIcon", new IMAGE_BRUSH("Icons/Materialize_Icon_40x", Icon40x40));
	Set("Materialize.ToolbarIcon.Small", new IMAGE_BRUSH("Icons/Materialize_Icon_16x", Icon16x16));
}

void FMaterializeStyle::Register()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = MakeShareable(new FMaterializeStyle());
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMaterializeStyle::Unregister()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		StyleInstance.Reset();
	}
}

const FMaterializeStyle& FMaterializeStyle::Get()
{
	if (!StyleInstance.IsValid())
	{
		Register();
	}
	return *StyleInstance;
}

const FName& FMaterializeStyle::GetStyleSetName() const
{
	return StyleSetName;
}

#undef IMAGE_BRUSH
