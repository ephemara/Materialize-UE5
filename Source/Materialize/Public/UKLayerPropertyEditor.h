#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "KLayerStack.h"
#include "TimerManager.h"
#include "Editor.h"
#include "UKLayerPropertyEditor.generated.h"

/**
 * Wrapper UObject that exposes an FKLayer struct to the Details View.
 * This allows editing layer properties through Unreal's standard property system.
 * Changes are automatically written back to the source layer stack.
 */
UCLASS(Transient)
class MATERIALIZE_API UKLayerPropertyEditor : public UObject
{
	GENERATED_BODY()

public:
	// The layer data being edited - exposed to Details View
	UPROPERTY(EditAnywhere, Category = "Layer")
	FKLayer Layer;

	// Source tracking for writeback
	int32 SourceIndex = INDEX_NONE;
	FKLayerStack* SourceStack = nullptr;

	// Delegate for notifying the editor when properties change
	DECLARE_MULTICAST_DELEGATE(FOnLayerPropertyChanged);
	FOnLayerPropertyChanged OnLayerPropertyChanged;

	/**
	 * Bind this editor to a specific layer in a stack
	 * @param Stack Pointer to the layer stack
	 * @param Index Index of the layer to edit
	 */
	void SetLayer(FKLayerStack* Stack, int32 Index)
	{
		SourceStack = Stack;
		SourceIndex = Index;

		if (Stack && Stack->Layers.IsValidIndex(Index))
		{
			Layer = Stack->Layers[Index];
		}
		else
		{
			Layer = FKLayer();
			SourceIndex = INDEX_NONE;
		}
	}

	/**
	 * Write edited values back to the source layer stack
	 */
	void WriteBack()
	{
		if (SourceStack && SourceStack->Layers.IsValidIndex(SourceIndex))
		{
			SourceStack->Layers[SourceIndex] = Layer;
		}
	}

	/**
	 * Clear the editor state
	 */
	void Clear()
	{
		CancelDebouncedBroadcast();
		SourceStack = nullptr;
		SourceIndex = INDEX_NONE;
		Layer = FKLayer();
	}

	/**
	 * Check if currently bound to a valid layer
	 */
	bool IsValid() const
	{
		return SourceStack != nullptr && SourceStack->Layers.IsValidIndex(SourceIndex);
	}

	// Override to detect property changes and write them back
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		// Always write changes back to source immediately so the stack is never stale,
		// even during interactive slider drags.
		WriteBack();

		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
		{
			// Interactive drag: schedule a debounced re-evaluation so we don't
			// hammer the GPU evaluator on every tick of a slider.
			ScheduleDebouncedBroadcast();
		}
		else
		{
			// Committed change (mouse release, text field confirm, etc.):
			// cancel any pending debounce and fire immediately.
			CancelDebouncedBroadcast();
			OnLayerPropertyChanged.Broadcast();
		}
	}

private:
	/** Timer handle used to debounce rapid interactive property changes. */
	FTimerHandle DebounceTimerHandle;

	/** Seconds to wait after the last interactive change before broadcasting. */
	static constexpr float DebounceDelaySec = 0.15f;

	void ScheduleDebouncedBroadcast()
	{
		if (GEditor)
		{
			// Reset the timer each call so only the final change in a burst fires.
			GEditor->GetTimerManager()->SetTimer(
				DebounceTimerHandle,
				FTimerDelegate::CreateUObject(this, &UKLayerPropertyEditor::FireDebouncedBroadcast),
				DebounceDelaySec,
				/*bLoop=*/false);
		}
	}

	void CancelDebouncedBroadcast()
	{
		if (GEditor)
		{
			GEditor->GetTimerManager()->ClearTimer(DebounceTimerHandle);
		}
	}

	void FireDebouncedBroadcast()
	{
		OnLayerPropertyChanged.Broadcast();
	}
};
