// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioSubsystem.h"
#include "AudioManager.generated.h"


UCLASS()
class AUDIOSYSTEMS_API AAudioManager : public AActor
{
	GENERATED_BODY()
	
public:
	AAudioManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere)
	USoundConcurrency* dxConcurrency;
	UPROPERTY(EditAnywhere)
	USoundAttenuation* dxAttenuation;
	
	UPROPERTY(EditAnywhere)
	TMap<ETag, FMusicTrack> MusicMap;
	
	UPROPERTY(EditAnywhere)
	TMap<ETag, F_2DAmbience> AmbienceMap;
};
