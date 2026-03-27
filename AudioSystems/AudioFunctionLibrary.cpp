// Fill out your copyright notice in the Description page of Project Settings.


#include "AudioFunctionLibrary.h"

void UAudioFunctionLibrary::QueueDialogueLine(const UObject* WorldContextObject, FDialogueLine line, EQueueType queueType)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->QueueDialogueLine(line, queueType);
}

void UAudioFunctionLibrary::QueueDialogueSequence(const UObject* WorldContextObject, TArray<FDialogueLine> lines, EQueueType queueType)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->QueueDialogueSequence(lines, queueType);
}

void UAudioFunctionLibrary::PreventAllDialogue(const UObject* WorldContextObject, float duration)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->SetPreventDialogue(duration);
}

void UAudioFunctionLibrary::InterruptAndClearQueues(const UObject* WorldContextObject)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->InterruptAndClearQueues();
}

void UAudioFunctionLibrary::ClearQueues(const UObject* WorldContextObject)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->ClearQueues();
}

void UAudioFunctionLibrary::InterruptCharacter(const UObject* WorldContextObject, ECharacters EChar)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->InterruptCharacter(EChar);
}

void UAudioFunctionLibrary::SetPreventCharacterSpeech(const UObject* WorldContextObject, ECharacters EChar, float time)
{
	UAudioSubsystem* audioSys = UAudioSubsystem::Get(WorldContextObject);
	if (ensure(audioSys))
		audioSys->SetPreventCharacterSpeech(EChar, time);
}


