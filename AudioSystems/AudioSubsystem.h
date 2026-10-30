// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "MetasoundSource.h"
//#include "Plugins/Runtime/Metasound/Source/MetasoundEngine/Public/MetasoundSource.h"
//#include "Metasound/Source/MetasoundEngine/Public/MetasoundSource.h"
//#include "Source/Runtime/AudioExtensions/AudioParameterControllerInterface.h" // correct?
#include "AudioParameterControllerInterface.h" // correct?
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h" 
#include "UObject/SoftObjectPath.h"
#include "Containers/Deque.h"
#include "AudioSubsystem.generated.h"



UENUM(BlueprintType)
enum class ETag : uint8
{
	Market, Gangster, Bebe, Warehouse, BullBoss, Rooftops, Desert
};


UENUM(BlueprintType)
enum class ECharacters : uint8
{
	NONE, CharacterOne, CharacterTwo, CharacterThree, CharacterFour
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line",
		meta=(ToolTip="It is not recommended to use MetaSounds for these"))
	USoundWave* Sound;

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
	int MaxTimesCanPlay = -1;

	int TimesPlayed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line",
		meta=(ToolTip="After this line has finished, there can be a time when no line can be played"))
	float Gap = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Line")
	bool AllowReverb = true;

	float TimeSpentInQueue = 0.f;
	float GameTimeQueued;
	float GameTimePlayed;
	bool isGroup = false;
	int groupId;
	UAudioComponent* instance;

	bool operator==(const FDialogueLine& Other) const {
		return (Sound == Other.Sound);
	};
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

USTRUCT(BlueprintType)
struct FSequenceContainer
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	TArray<USoundBase*> sounds;
};

UENUM(BlueprintType)
enum class ERandomType : uint8
{
	Standard, Shuffle
};

USTRUCT(BlueprintType)
struct FRandomContainer
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	TArray<USoundBase*> sounds;

	//UPROPERTY(EditAnywhere)
	//ERandomType randomType;

	UPROPERTY(EditAnywhere)
	bool limitRepetitions;

	//UPROPERTY(EditAnywhere)
	//int limitRepetitionsTo;
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

	UFUNCTION(BlueprintCallable)
	void InitAudioData(TMap<ETag, FMusicTrack> MusicMap,
		TMap<ETag, F_2DAmbience> AmbienceMap,
		USoundConcurrency* DialogueConcurrency,
		USoundAttenuation* DialogueAttenuation);

protected:
	void ProcessDialogueQueue();
	void ProcessDialogueDelayQueue(float DeltaTime);
	void RemoveLineAndItsSequencePartners(int queuePos, bool isGroup, int groupId);
	void RemoveLineAndPartnersFromDelayQueue(int queuePos, bool isGroup, int groupId);
	bool IsLineValidAccordingToHistory(const FDialogueLine& Line, float gameTimeNow);
	void PreQueueAction(EQueueType queueType);
	void AddLineToLastLineFromCharacterMap(FDialogueLine& Line);

protected:
	TArray<FDialogueLine> m_DialogueQueue;
	TArray<FDialogueLine> m_DialogueDelayQueue; // Not necessarily first in first out as it depends on PlayDelay
	TMap<USoundBase*, FDialogueLine> m_DialogueHistory;

	UAudioComponent* m_CurrentlyPlayingDialogue;
	TMap<ECharacters, FDialogueLine> m_LastLineFromCharacterMap;
	TMap<ECharacters, float> m_CharacterPreventMap;

	void DebugPrintDxSystem(FString msg, bool cacheMe);
	void DebugPrintDialogueQueue(bool preventActive);
	FString m_DebugCachedMsg = "";
	float m_CachedMsgMaxTime = 1.f;
	float m_CachedMsgTime = 0.f;

	float m_PreventDialogue = 0.f;
	float m_DefaultGapBetweenLines = 0.8f;

	int m_NextGroupId = 0;

	/// 
	/// Music
	/// 

	FMusicTrack m_CurrentlyPlayingMusic;
	UFUNCTION(BlueprintCallable)
	FMusicTrack& GetCurrentlyPlayingMusic();
	void PlayNewMusic(FMusicTrack Track, USoundBase* LoadedSound);
	void DebugPrintMusicSystem(FString msg, bool GameTime);

	UFUNCTION(BlueprintCallable)
	void LoadAndPlayNewMusic(FMusicTrack Track);

	UFUNCTION(BlueprintCallable)
	void CurrentMusicTriggerParameter(FString parameter);

	UFUNCTION(BlueprintCallable)
	void CurrentMusicTriggerFloatParameter(FString parameter, float value);


	///// Ambience /////

	F_2DAmbience m_CurrentlyPlayingAmbience;

	void ResetTimerForIntermittentAmbienceElements();
	void AttemptIntermittentAmbSound();

	void PlayNewAmbience(F_2DAmbience ambience, USoundBase* Sound);



	TMap<ETag, F_2DAmbience> m_AmbienceMap;

	UFUNCTION(BlueprintCallable)
	void EvaluateAmbienceMap(ETag tag);

	UFUNCTION(BlueprintCallable)
	void LoadNewAmbience(F_2DAmbience ambience);

	UFUNCTION(BlueprintCallable)
	F_2DAmbience& GetCurrentlyPlayingAmbience();

	UFUNCTION(BlueprintCallable)
	void StopAllAmbience(float fadeTime, EAudioFaderCurve curve);

public:


	////// DIALOGUE //////

	UFUNCTION(BlueprintCallable)
	void QueueDialogueLine(FDialogueLine line, EQueueType queueType);

	UFUNCTION(BlueprintCallable)
	void QueueDialogueSequence(TArray<FDialogueLine> lines, EQueueType queueType);

	UFUNCTION(BlueprintCallable)
	void SetPreventDialogue(float duration);

	UFUNCTION(BlueprintCallable)
	void InterruptAndClearQueues(float fadeTime);

	UFUNCTION(BlueprintCallable)
	void ClearQueues();

	UFUNCTION(BlueprintCallable)
	void InterruptCharacter(ECharacters EChar);

	UFUNCTION(BlueprintCallable)
	void SetPreventCharacterSpeech(ECharacters EChar, float time);

protected:
	
	USoundConcurrency* m_DxConcurrency;
	USoundAttenuation* m_DxAttenuation;
	TMap<ETag, FMusicTrack> m_MusicMap;

public:


	UFUNCTION(BlueprintCallable)
	void EvaluateMusicMap(ETag tag);

	UFUNCTION(BlueprintCallable)
	void ReconcileNewMusic(FMusicTrack musicTrack);

	UFUNCTION(BlueprintCallable)
	void StopMusic(float fadeTime, EAudioFaderCurve curveType);

/// SFX ///

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	void PlaySoundSequenceContainer2D(UObject* WorldContextObject, FSequenceContainer sounds, float vol = 1.f, float pitch = 1.f);

	UFUNCTION(BlueprintCallable)
	void ResetSequenceContainer(FSequenceContainer sounds);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	void PlaySoundRandomContainer2D(UObject* WorldContextObject, FRandomContainer sounds, float vol = 1.f, float pitch = 1.f);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	void PlaySound2DManaged(UObject* WorldContextObject, USoundBase* sound, float vol = 1.f, float pitch = 1.f, bool safe = false);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	void SpawnSound2DManaged(UObject* WorldContextObject, USoundBase* sound, float vol = 1.f, float pitch = 1.f, bool safe = false);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	void PlaySoundAtLocationManaged(UObject* WorldContextObject, USoundBase* sound, FVector location, float vol = 1.f, float pitch = 1.f, bool safe = false);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	void SpawnSoundAtLocationManaged(UObject* WorldContextObject, USoundBase* sound, FVector location, float vol = 1.f, float pitch = 1.f, bool safe = false);
private:

	USoundBase* SequenceSFXParent(FRandomContainer sounds, float vol = 1.f, float pitch = 1.f);

	TMap<USoundBase*, float> m_SafeSoundHistory;
	float m_SafeSoundThreshold = 0.02f;

	TMap<USoundBase*, int> m_SoundSequencesTracker;
	//TArray<FSequenceContainer> m_TrackedSequences; // TODO DL delete if unused

	TMap<USoundBase*, TArray<USoundBase*>> m_SoundRandomContainersTracker;
	TMap<USoundBase*, TArray<USoundBase*>> m_SoundRandomContainersShuffleTracker;

protected:
	bool m_DEBUG_DIALOGUE = true;
	bool m_DEBUG_MUSIC = true;
};