// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AudioSubsystem.h"
#include "AudioFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AUDIOSYSTEMS_API UAudioFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void QueueDialogueLine(const UObject* WorldContextObject, FDialogueLine line, EQueueType queueType);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void QueueDialogueSequence(const UObject* WorldContextObject, TArray<FDialogueLine> lines, EQueueType queueType);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void PreventAllDialogue(const UObject* WorldContextObject, float duration);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", ToolTip="Current dialogue will be interrupted"))
	static void InterruptAndClearQueues(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", ToolTip="Current dialogue will finish playing"))
	static void ClearQueues(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void InterruptCharacter(const UObject* WorldContextObject, ECharacters EChar);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void SetPreventCharacterSpeech(const UObject* WorldContextObject, ECharacters EChar, float time);
};
