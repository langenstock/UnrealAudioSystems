#include "AudioManager.h"
// Fill out your copyright notice in the Description page of Project Settings.





AAudioManager::AAudioManager()
{
	PrimaryActorTick.bCanEverTick = true;
	//DialogueHistory.Empty(200);
}


void AAudioManager::BeginPlay()
{
	Super::BeginPlay();

	UObject* context = this;
	UAudioSubsystem* subsys = UAudioSubsystem::Get(context);
	// Level specific data can be set on the AudioManager and the subsystem
	// will be updated with that data here
	subsys->InitAudioData(MusicMap, AmbienceMap, dxConcurrency, dxAttenuation);
}

void AAudioManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}