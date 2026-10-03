#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"

/**
 * FMaterializeStyle
 * 
 * Manages custom Slate styles and icons for the Materialize plugin.
 * Registers custom toolbar icons and other UI assets.
 */
class FMaterializeStyle : public FSlateStyleSet
{
public:
	/**
	 * Register the style set with Slate.
	 * Should be called during module startup.
	 */
	static void Register();
	
	/**
	 * Unregister the style set.
	 * Should be called during module shutdown.
	 */
	static void Unregister();
	
	/**
	 * Get the singleton style instance.
	 * @return Reference to the style set
	 */
	static const FMaterializeStyle& Get();
	
	/**
	 * Get the style set name.
	 * @return The name of this style set
	 */
	virtual const FName& GetStyleSetName() const override;

private:
	/** Private constructor - use Get() to access singleton */
	FMaterializeStyle();
	
	/** Singleton instance */
	static TSharedPtr<FMaterializeStyle> StyleInstance;
};
