// Fill out your copyright notice in the Description page of Project Settings.


#include "AudioSubsystem.h"


void UAudioSubsystem::Tick(float DeltaTime)
{
	if (!GetWorld()) { return; };
}

TStatId UAudioSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAudioSubsystem, STATGROUP_Tickables);
}

UAudioSubsystem* UAudioSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* w = WorldContextObject->GetWorld();
	return w->GetGameInstance()->GetSubsystem<UAudioSubsystem>();
}

