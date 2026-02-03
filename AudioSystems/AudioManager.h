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
	// Sets default values for this actor's properties
	AAudioManager();



protected:


	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TArray<FDialogueLine> DialogueQueue;
	TArray<FDialogueLine> DialogueDelayQueue;
	TMap<USoundBase*, FDialogueLine> DialogueHistory;

	UAudioComponent* CurrentlyPlayingDialogue;
	TMap<ECharacters, FDialogueLine> LastLineFromCharacterMap;
	TMap<ECharacters, float> CharacterPreventMap;

	void ProcessDialogueQueue();
	void ProcessDialogueDelayQueue(float DeltaTime);
	void RemoveLineAndItsSequencePartners(int queuePos, bool isGroup, int groupId);
	void RemoveLineAndPartnersFromDelayQueue(int queuePos, bool isGroup, int groupId);
	bool IsLineValidAccordingToHistory(const FDialogueLine& Line, float gameTimeNow);
	void PostQueueTypeAction(EQueueType queueType);
	void AddLineToLastLineFromCharacterMap(FDialogueLine& Line);

	void DebugPrintDxSystem(FString msg, bool cacheMe);
	void DebugPrintDialogueQueue(bool preventActive);
	FString DebugCachedMsg = "";
	float CachedMsgMaxTime = 1.f;
	float CachedMsgTime = 0.f;

	float PreventDialogue = 0.f;
	float DefaultGapBetweenLines = 0.8f;

	int nextGroupId = 0;
	
	/// 
	/// Music
	/// 

	FMusicTrack CurrentlyPlayingMusic;
	UFUNCTION(BlueprintCallable)
	FMusicTrack& GetCurrentlyPlayingMusic();
	void PlayNewMusic(FMusicTrack Track, USoundBase* LoadedSound);
	void DebugPrintMusicSystem(FString msg, bool GameTime);


	///// Ambience /////

	F_2DAmbience CurrentlyPlayingAmbience;
	
	void ResetTimerForIntermittentAmbienceElements();
	void AttemptIntermittentAmbSound();

	void PlayNewAmbience(F_2DAmbience ambience, USoundBase* Sound);

	bool DEBUG_DIALOGUE = true;
	bool DEBUG_MUSIC = true;

public:
	virtual void Tick(float DeltaTime) override;

	////// DIALOGUE //////

	UFUNCTION(BlueprintCallable)
	void QueueDialogueLine(FDialogueLine line, EQueueType queueType);

	UFUNCTION(BlueprintCallable)
	void QueueDialogueSequence(TArray<FDialogueLine> lines, EQueueType queueType);

	UFUNCTION(BlueprintCallable)
	void SetPreventDialogue(float duration);

	UFUNCTION(BlueprintCallable)
	void InterruptAndClearQueues();

	UFUNCTION(BlueprintCallable)
	void ClearQueues();

	UFUNCTION(BlueprintCallable)
	void InterruptCharacter(ECharacters EChar);

	UFUNCTION(BlueprintCallable)
	void SetPreventCharacterSpeech(ECharacters EChar, float time);

	UPROPERTY(EditAnywhere)
	USoundConcurrency* dxConcurrency;
	UPROPERTY(EditAnywhere)
	USoundAttenuation* dxAttenuation;

	///// Music /////
	
	UPROPERTY(EditAnywhere)
	TMap<ETag, FMusicTrack> MusicMap;

	UFUNCTION(BlueprintCallable)
	void EvaluateMusicMap(ETag tag);

	UFUNCTION(BlueprintCallable)
	void ReconcileNewMusic(FMusicTrack musicTrack);

	UFUNCTION(BlueprintCallable)
	void StopMusic(float fadeTime, EAudioFaderCurve curveType);

	///// Ambience /////

	UPROPERTY(EditAnywhere)
	TMap<ETag, F_2DAmbience> AmbienceMap;

	UFUNCTION(BlueprintCallable)
	void EvaluateAmbienceMap(ETag tag);

	UFUNCTION(BlueprintCallable)
	void LoadNewAmbience(F_2DAmbience ambience);

	UFUNCTION(BlueprintCallable)
	F_2DAmbience& GetCurrentlyPlayingAmbience();

	UFUNCTION(BlueprintCallable)
	void StopAllAmbience(float fadeTime, EAudioFaderCurve curve);


};
