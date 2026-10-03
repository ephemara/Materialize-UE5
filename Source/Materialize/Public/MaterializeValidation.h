#pragma once

#include "CoreMinimal.h"

// Declare the log category for use across the plugin
DECLARE_LOG_CATEGORY_EXTERN(LogMaterialize, Log, All);

/**
 * Crash prevention validation macros for Materialize plugin
 * These macros provide consistent null checking, bounds validation, and error logging
 * to prevent crashes during material loading and processing operations.
 */

/**
 * Validate a pointer is not null before dereferencing
 * Logs an error and returns the specified value if the pointer is null
 * 
 * Usage:
 *   MATERIALIZE_CHECK_PTR(MyPointer, false);
 *   MATERIALIZE_CHECK_PTR(MyObject, nullptr);
 */
#define MATERIALIZE_CHECK_PTR(Ptr, ReturnValue) \
	if (!(Ptr)) { \
		UE_LOG(LogMaterialize, Error, TEXT("Null pointer: %s at %s:%d"), \
			TEXT(#Ptr), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

/**
 * Validate a UObject is valid (not null, not pending kill, and passes IsValid check)
 * Logs an error and returns the specified value if the object is invalid
 * 
 * Usage:
 *   MATERIALIZE_CHECK_UOBJECT(MyMaterial, false);
 *   MATERIALIZE_CHECK_UOBJECT(MyTexture, nullptr);
 */
#define MATERIALIZE_CHECK_UOBJECT(Obj, ReturnValue) \
	if (!IsValid(Obj)) { \
		UE_LOG(LogMaterialize, Error, TEXT("Invalid UObject: %s at %s:%d"), \
			TEXT(#Obj), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

/**
 * Validate an array index is within valid bounds
 * Logs an error and returns the specified value if the index is out of bounds
 * 
 * Usage:
 *   MATERIALIZE_CHECK_BOUNDS(MyArray, Index, false);
 *   MATERIALIZE_CHECK_BOUNDS(Textures, i, nullptr);
 */
#define MATERIALIZE_CHECK_BOUNDS(Array, Index, ReturnValue) \
	if (!(Array).IsValidIndex(Index)) { \
		UE_LOG(LogMaterialize, Error, TEXT("Index %d out of bounds for %s (size %d) at %s:%d"), \
			Index, TEXT(#Array), (Array).Num(), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

/**
 * Validate a pointer is not null before dereferencing (void return version)
 * Logs an error and returns early if the pointer is null
 * 
 * Usage:
 *   MATERIALIZE_CHECK_PTR_VOID(MyPointer);
 */
#define MATERIALIZE_CHECK_PTR_VOID(Ptr) \
	if (!(Ptr)) { \
		UE_LOG(LogMaterialize, Error, TEXT("Null pointer: %s at %s:%d"), \
			TEXT(#Ptr), TEXT(__FILE__), __LINE__); \
		return; \
	}

/**
 * Validate a UObject is valid (void return version)
 * Logs an error and returns early if the object is invalid
 * 
 * Usage:
 *   MATERIALIZE_CHECK_UOBJECT_VOID(MyMaterial);
 */
#define MATERIALIZE_CHECK_UOBJECT_VOID(Obj) \
	if (!IsValid(Obj)) { \
		UE_LOG(LogMaterialize, Error, TEXT("Invalid UObject: %s at %s:%d"), \
			TEXT(#Obj), TEXT(__FILE__), __LINE__); \
		return; \
	}

/**
 * Validate an array index is within valid bounds (void return version)
 * Logs an error and returns early if the index is out of bounds
 * 
 * Usage:
 *   MATERIALIZE_CHECK_BOUNDS_VOID(MyArray, Index);
 */
#define MATERIALIZE_CHECK_BOUNDS_VOID(Array, Index) \
	if (!(Array).IsValidIndex(Index)) { \
		UE_LOG(LogMaterialize, Error, TEXT("Index %d out of bounds for %s (size %d) at %s:%d"), \
			Index, TEXT(#Array), (Array).Num(), TEXT(__FILE__), __LINE__); \
		return; \
	}
