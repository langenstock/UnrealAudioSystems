// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h" 
#include "UObject/SoftObjectPath.h"
#include "AudioSubsystem.generated.h"



UENUM(BlueprintType)
enum class ETag : uint8
{
	Market, Gangster, Bibi, Warehouse, BullBoss, Rooftops, Desert
};


UENUM(BlueprintType)
enum class ECharacters : uint8
{
	NONE, Fang, Maisie, Buster, Lola
};

UENUM(BlueprintType)
enum class EQueueType : uint8
{
	Queue, QueuePlayNext, QueuePlayNow,
	NONE UMETA(Hidden)
};



USTRUCT(BlueprintType)
struct FDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	USoundBase* Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	ECharacters EChar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	float PlayDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	float MaxTimeCanBeInQueue = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	float Cooldown = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	bool InfiniteCooldown = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	int MaxtimesCanPlay = -1;

	int TimesPlayed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	float Gap = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	bool AllowReverb = true;

	float TimeSpentInQueue = 0.f;
	float GameTimeQueued;
	float GameTimePlayed;
	bool isGroup = false;
	int groupId;
	UAudioComponent* instance;
};

USTRUCT(BlueprintType)
struct FMusicTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	USoundBase* SoundHard;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USoundBase> SoundSoft;

	UPROPERTY(EditAnywhere)
	float Volume = 1.f;

	UPROPERTY(EditAnywhere)
	bool TransitionAcrossLevels = false;

	UPROPERTY(EditAnywhere)
	float PreviousTrackFadeOut = 0.5f;

	UPROPERTY(EditAnywhere)
	EAudioFaderCurve FadeOutType;

	UPROPERTY(EditAnywhere)
	float NewTrackFadeIn = 0.1f;

	UPROPERTY(EditAnywhere)
	EAudioFaderCurve FadeInType;

	UPROPERTY(EditAnywhere)
	int32 TimeSignatureBeats = 4;

	UAudioComponent* instance;
};


USTRUCT(BlueprintType)
struct F_2DAmbience
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere)
	float Volume = 1.f;

	UPROPERTY(EditAnywhere)
	float FadeInTime = 2.f;

	UPROPERTY(EditAnywhere)
	EAudioFaderCurve FadeInType = EAudioFaderCurve::Sin;

	UPROPERTY(EditAnywhere)
	float FadeOutTime = 2.f;

	UPROPERTY(EditAnywhere)
	EAudioFaderCurve FadeOutType = EAudioFaderCurve::Sin;

	UAudioComponent* instance;

	UPROPERTY(EditAnywhere, meta = (Tooltip = "Can be left blank"))
	TSoftObjectPtr<USoundBase> IntermittentElements;

	UPROPERTY(EditAnywhere)
	float IntElementsVolume = 1.f;

	UPROPERTY(EditAnywhere)
	float TriggerTimeMinimum = 5.f;

	UPROPERTY(EditAnywhere)
	float TriggerTimeMaximum = 10.f;

	float timeUntilNextIntermittent;

	UAudioComponent* intElementsInstance;

	USoundBase* LoadedIntElement;
};


UCLASS()
class AUDIOSYSTEMS_API UAudioSubsystem : public UGameInstanceSubsystem,
	public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject", DisplayName = "Get Audio Subsystem"))
	static UAudioSubsystem* Get(const UObject* WorldContextObject);

};