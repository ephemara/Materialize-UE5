#pragma once

#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

/**
 * FMaterializeToolbarExtension
 * 
 * Manages toolbar integration for the Materialize plugin.
 * Adds a toolbar button to the main Unreal Editor toolbar for quick access to the Materialize editor.
 */
class FMaterializeToolbarExtension
{
public:
	/**
	 * Register the toolbar extension with the Level Editor.
	 * Should be called during module startup.
	 */
	static void RegisterToolbarExtension();
	
	/**
	 * Unregister the toolbar extension.
	 * Should be called during module shutdown.
	 */
	static void UnregisterToolbarExtension();

private:
	/**
	 * Generate the toolbar button widget.
	 * @return The toolbar button widget
	 */
	static TSharedRef<SWidget> GenerateToolbarButton();
	
	/**
	 * Handle toolbar button click event.
	 * Opens the Materialize editor tab.
	 */
	static void OnToolbarButtonClicked();
	
	/**
	 * Get the icon for the toolbar button.
	 * @return The slate icon for the button
	 */
	static FSlateIcon GetToolbarIcon();

private:
	/** Toolbar extender instance */
	static TSharedPtr<FExtender> ToolbarExtender;
	
	/** Extension delegate handle for cleanup */
	static FDelegateHandle ExtensionHandle;
};
