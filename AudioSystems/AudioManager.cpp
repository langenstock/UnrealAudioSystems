#include "AudioManager.h"
// Fill out your copyright notice in the Description page of Project Settings.





AAudioManager::AAudioManager()
{
	PrimaryActorTick.bCanEverTick = true;
	DialogueHistory.Empty(200);
}
/*

AAudioManager* AAudioManager::Get() {
	if (!IsValid(audioManagerInstance)) {
		UWorld* w = GetWorld();
		if (!w) { return nullptr; };

		FVector l{ 0, 0, 0 };
		FRotator r{ 0, 0, 0 };
		FActorSpawnParameters params{};
		audioManagerInstance = w->SpawnActor<AAudioManager>(AAudioManager::StaticClass(), l, r, params);
	}
	return audioManagerInstance;
}
*/

void AAudioManager::BeginPlay()
{
	Super::BeginPlay();
}


void AAudioManager::ResetTimerForIntermittentAmbienceElements()
{
	float r = FMath::FRandRange(CurrentlyPlayingAmbience.TriggerTimeMinimum, CurrentlyPlayingAmbience.TriggerTimeMaximum);
	CurrentlyPlayingAmbience.timeUntilNextIntermittent = r;
}


void AAudioManager::AttemptIntermittentAmbSound()
{
	// Not a typo - we only play intermittents if there is no dialogue playing
	if (CurrentlyPlayingDialogue && CurrentlyPlayingDialogue->IsPlaying())
		return;
	
	UAudioComponent* s = UGameplayStatics::SpawnSound2D(GetWorld(), CurrentlyPlayingAmbience.LoadedIntElement, 1.f, 1.f);
	if (s) {
		CurrentlyPlayingAmbience.intElementsInstance = s;
	}
}


void AAudioManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DialogueQueue.IsEmpty() == false) {
		if (DEBUG_DIALOGUE)	{
			DebugPrintDialogueQueue((PreventDialogue > 0.f));
			if (CachedMsgTime > 0.f)
			{
				CachedMsgTime -= DeltaTime;
			}
		}
		ProcessDialogueQueue();
	}

	if (PreventDialogue > 0.f) {
		PreventDialogue -= DeltaTime;
	}

	ProcessDialogueDelayQueue(DeltaTime);

	if (CurrentlyPlayingAmbience.IntermittentElements) {
		CurrentlyPlayingAmbience.timeUntilNextIntermittent -= DeltaTime;
		if (CurrentlyPlayingAmbience.timeUntilNextIntermittent < 0.f) {
			AttemptIntermittentAmbSound();
			ResetTimerForIntermittentAmbienceElements();
		}
	}
}

void AAudioManager::ProcessDialogueDelayQueue(float DeltaTime)
{
	TArray<int> indicesToMove;
	size_t j = DialogueDelayQueue.Num();

	for (size_t i = 0; i < j; i++) {
		FDialogueLine& line = DialogueDelayQueue[i];
		line.TimeSpentInQueue += DeltaTime;
		if (line.TimeSpentInQueue > line.PlayDelay)	{
			indicesToMove.Push(i);
		}
	}

	for (int& i : indicesToMove) {
		DialogueQueue.Push(DialogueDelayQueue[i]);
		DialogueDelayQueue.RemoveAt(i);
	}
}

void AAudioManager::ProcessDialogueQueue()
{
	if (PreventDialogue > 0.f)
		return;

	float gameTimeNow = (float)GetWorld()->GetTimeSeconds();

	FDialogueLine thisLine = DialogueQueue[0];
	bool isGroup = thisLine.isGroup;
	int groupId = thisLine.groupId;
	ECharacters thisLineChar = thisLine.EChar;

	if (IsLineValidAccordingToHistory(thisLine, gameTimeNow) == false) {
		RemoveLineAndItsSequencePartners(0, isGroup, groupId);
		return;
	}

	// if MaxTime is -1 we ignore this
	if (thisLine.MaxTimeCanBeInQueue > 0.f) {
		if (gameTimeNow > (thisLine.GameTimeQueued + thisLine.MaxTimeCanBeInQueue))	{
			if (DEBUG_DIALOGUE) {
				FString msg = thisLine.Sound->GetName();
				msg += " was not queued (max time in queue passed)";
				DebugPrintDxSystem(msg, true);
			}
			RemoveLineAndItsSequencePartners(0, isGroup, groupId);
			return;
		}
	}

	if (CharacterPreventMap.Find(thisLineChar)) {
		// is game time free greater than game time now?
		if (CharacterPreventMap[thisLineChar] > gameTimeNow) {
			if (DEBUG_DIALOGUE)	{
				FString msg = thisLine.Sound->GetName();
				msg += " was not queued (character is still prevented)";
				DebugPrintDxSystem(msg, true);
			}
			RemoveLineAndItsSequencePartners(0, isGroup, groupId);
			return;
		}
	}

	FVector loc = FVector(0.f, 0.f, 0.f);
	FRotator r = FRotator(0.f, 0.f, 0.f);
	float vol = 1.f;
	float pitch = 1.f;
	float time = 0.f; // startTime in wave file

	USoundAttenuation* att = thisLine.AllowReverb ? dxAttenuation : nullptr;

	USoundBase* Sound = thisLine.Sound;
	UAudioComponent* LinePlaying = UGameplayStatics::SpawnSoundAtLocation(this, Sound, loc, r, vol,
		pitch, time, att,
		dxConcurrency);
	if (LinePlaying && LinePlaying->IsPlaying()) {
		CurrentlyPlayingDialogue = LinePlaying;
		thisLine.instance = LinePlaying;

		float lineDuration = Sound->GetDuration();
		float gap = thisLine.Gap;
		if (gap == 0.f)
			gap = DefaultGapBetweenLines;

		DialogueQueue.RemoveAt(0); // removing because it was successful

		SetPreventDialogue(lineDuration + gap);

		if (DialogueHistory.Find(Sound) == nullptr)	{
			DialogueHistory.Add(TTuple<USoundBase*, FDialogueLine>(Sound, thisLine));
		}
		DialogueHistory[Sound].GameTimePlayed = gameTimeNow;
		DialogueHistory[Sound].TimesPlayed++;

		AddLineToLastLineFromCharacterMap(thisLine);
	}
}


void AAudioManager::RemoveLineAndItsSequencePartners(int queuePos, bool isGroup, int groupId)
{
	if (isGroup) {
		int size = DialogueQueue.Num();
		for (int i = size - 1; i >= 0; i--)	{
			if (DialogueQueue[i].groupId == groupId) {
				DialogueQueue.RemoveAt(i);
			}
		}

		size = DialogueDelayQueue.Num();
		for (int i = size - 1; i >= 0; i--)	{
			if (DialogueDelayQueue[i].groupId == groupId) {
				DialogueDelayQueue.RemoveAt(i);
			}
		}
	}
	else {
		DialogueQueue.RemoveAt(queuePos);
	}
}

void AAudioManager::RemoveLineAndPartnersFromDelayQueue(int queuePos, bool isGroup, int groupId)
{
	if (isGroup) {
		int size = DialogueDelayQueue.Num();
		for (int i = size - 1; i >= 0; i--)	{
			if (DialogueDelayQueue[i].groupId == groupId) {
				DialogueDelayQueue.RemoveAt(i);
			}
		}
	}
	else {
		DialogueDelayQueue.RemoveAt(queuePos);
	}
}

bool AAudioManager::IsLineValidAccordingToHistory(const FDialogueLine& Line, float gameTimeNow)
{
	if (DialogueHistory.Find(Line.Sound)) {
		FDialogueLine& LineInHistory = DialogueHistory[Line.Sound];
		if (LineInHistory.InfiniteCooldown)	{
			if (DEBUG_DIALOGUE)	{
				FString msg = Line.Sound->GetName();
				msg += " was not queued (infinite cooldown)";
				DebugPrintDxSystem(msg, true);
			}
			return false;
		}
		else if (LineInHistory.Cooldown > (gameTimeNow - LineInHistory.GameTimePlayed)) {
			if (DEBUG_DIALOGUE) {
				FString msg = Line.Sound->GetName();
				msg += " was not queued (still on cooldown)";
				DebugPrintDxSystem(msg, true);
			}
			return false;
		}
		if (LineInHistory.MaxtimesCanPlay > 0) { // if this value is -1 there is no limit
			if (LineInHistory.TimesPlayed >= LineInHistory.MaxtimesCanPlay) {
				if (DEBUG_DIALOGUE) {
					FString msg = Line.Sound->GetName();
					msg += " was not queued (exceeds max times can play)";
					DebugPrintDxSystem(msg, true);
				}
				return false;
			}
		}
	}
	return true;
}


void AAudioManager::QueueDialogueLine(FDialogueLine line, EQueueType queueType)
{
	line.GameTimeQueued = (float)GetWorld()->GetTimeSeconds();
	if (line.PlayDelay > 0.f) {
		DialogueDelayQueue.Push(line);
	}
	else {
		DialogueQueue.Push(line);
	}

	PostQueueTypeAction(queueType);
}


void AAudioManager::PostQueueTypeAction(EQueueType queueType)
{
	switch (queueType) {
	case(EQueueType::Queue):
		break;
	case(EQueueType::QueuePlayNow):
		InterruptAndClearQueues();
		break;
	case(EQueueType::QueuePlayNext):
		ClearQueues();
		break;
	case(EQueueType::NONE):
		break;
	}
}


void AAudioManager::AddLineToLastLineFromCharacterMap(FDialogueLine& Line)
{
	LastLineFromCharacterMap.Add({ Line.EChar, Line });
}


void AAudioManager::QueueDialogueSequence(TArray<FDialogueLine> lines, EQueueType queueType)
{
	// Evaluate validity of line 0
	FDialogueLine& LineZero = lines[0];
	FDialogueLine* LineZeroInHistory = DialogueHistory.Find(LineZero.Sound);

	if (LineZeroInHistory) {
		if (LineZeroInHistory->InfiniteCooldown == true) {
			if (DEBUG_DIALOGUE) {
				FString msg = LineZero.Sound->GetName();
				msg += " was not queued (infinite cooldown)";
				DebugPrintDxSystem(msg, true);
			}
			return;		// i.e. dont bother trying to queue any of this sequence

		}
		else if (LineZeroInHistory->MaxtimesCanPlay > 0) {
			if (LineZeroInHistory->TimesPlayed >= LineZeroInHistory->MaxtimesCanPlay) {
				if (DEBUG_DIALOGUE) {
					FString msg = LineZero.Sound->GetName();
					msg += " was not queued (max times can play value)";
					DebugPrintDxSystem(msg, true);
				}
				return;
			}
		}
	}

	float playDelay = LineZero.PlayDelay;
	bool infCooldown = LineZero.InfiniteCooldown;
	float cooldown = LineZero.Cooldown;
	int maxTimesCanPlay = LineZero.MaxtimesCanPlay;

	float runningDuration = playDelay + LineZero.Sound->GetDuration();

	if (LineZero.MaxTimeCanBeInQueue < 0.f) {
		LineZero.MaxTimeCanBeInQueue = runningDuration;
		// otherwise subsequent lines could potentially time out leaving line zero behind
	}

	for (auto& l : lines) {
		if (CharacterPreventMap.Find(l.EChar)) {
			if (CharacterPreventMap[l.EChar] > (GetWorld()->GetTimeSeconds())) {
				if (DEBUG_DIALOGUE) {
					FString msg = LineZero.Sound->GetName();
					msg += " was not queued (one of the characters in the sequence is on time out (prevented))";
					DebugPrintDxSystem(msg, true);
				}
				return;
				// i.e. one of the characters in the sequence is prevented, so ditch the whole seq
			}
		}

		if (l.Sound != LineZero.Sound) {// not the first line in the sequence
			float thisLineDur = l.Sound->GetDuration();
			float thisLineGap = (l.Gap <= 0.f) ? DefaultGapBetweenLines : l.Gap;
			l.PlayDelay = playDelay;
			runningDuration = runningDuration + thisLineDur + thisLineGap;
		}
	}

	int thisGroupId = ++nextGroupId;

	for (auto& l : lines) { 
		// For line[0] the max time is what is specified, for other lines, it is the calc'd duration of the seq
		if (l.Sound != LineZero.Sound) {
			l.MaxTimeCanBeInQueue = runningDuration;
		}

		l.MaxtimesCanPlay = maxTimesCanPlay;
		l.InfiniteCooldown = infCooldown;
		l.Cooldown = cooldown;
		l.isGroup = true;
		l.groupId = thisGroupId;
		QueueDialogueLine(l, EQueueType::NONE);
	}

	PostQueueTypeAction(queueType);
}

void AAudioManager::SetPreventDialogue(float duration)
{
	PreventDialogue = duration;
}

void AAudioManager::InterruptAndClearQueues()
{
	ClearQueues();

	if (CurrentlyPlayingDialogue) {
		if (CurrentlyPlayingDialogue->IsPlaying()) {
			CurrentlyPlayingDialogue->Stop();
		}
	}

	if (DEBUG_DIALOGUE)	{
		FString msg = "Interrupt and clear queues called";
		DebugPrintDxSystem(msg, true);
	}
}

void AAudioManager::ClearQueues()
{
	DialogueQueue.Empty();
	DialogueDelayQueue.Empty();
}

void AAudioManager::InterruptCharacter(ECharacters EChar)
{
	bool origLineIsGroup = false;
	int origLineGroupId = 0;
	if (LastLineFromCharacterMap.Find(EChar)) {
		FDialogueLine& lineToStop = LastLineFromCharacterMap[EChar];
		UAudioComponent* sound = lineToStop.instance;
		if (sound && sound->IsPlaying()) {
			if (lineToStop.isGroup)	{
				origLineGroupId = lineToStop.groupId;
				origLineIsGroup = true;
			}
			sound->Stop();
			if (DEBUG_DIALOGUE)	{
				FString msg = lineToStop.Sound->GetName();
				msg += " was interrupted (interrupt character)";
				DebugPrintDxSystem(msg, true);
			}
		}
	}

	int size = DialogueQueue.Num();
	for (int i = size - 1; i >= 0; i--)	{
		if (DialogueQueue.Num() > i) { // because the below functions may also remove elements
			if (origLineIsGroup) {
				if (DialogueQueue[i].isGroup && DialogueQueue[i].groupId == origLineGroupId) {
					RemoveLineAndItsSequencePartners(i, origLineIsGroup, origLineGroupId);
				}
			}
			else if (DialogueQueue[i].EChar == EChar) {
				bool isGrp = DialogueQueue[i].isGroup;
				int grpId = DialogueQueue[i].groupId;
				RemoveLineAndItsSequencePartners(i, isGrp, grpId);
			}
		}
	}

	size = DialogueDelayQueue.Num();
	for (int i = size - 1; i >= 0; i--) {
		if (DialogueDelayQueue.Num() > i) { // because the below functions may also remove elements
			if (DialogueDelayQueue[i].EChar == EChar) {
				bool isGroup = DialogueDelayQueue[i].isGroup;
				int groupId = DialogueDelayQueue[i].groupId;
				RemoveLineAndPartnersFromDelayQueue(i, isGroup, groupId);
			}
		}
	}
}

void AAudioManager::SetPreventCharacterSpeech(ECharacters EChar, float time) 
{
	float gt = GetWorld()->GetTimeSeconds();
	float gameTimeWillBeFree = gt + time;
	CharacterPreventMap.Add({ EChar, gameTimeWillBeFree });
}


FMusicTrack& AAudioManager::GetCurrentlyPlayingMusic()
{
	return CurrentlyPlayingMusic;
}


void AAudioManager::EvaluateMusicMap(ETag tag)
{
	if (MusicMap.Find(tag))	{
		FMusicTrack& musicTrack = MusicMap[tag];
		ReconcileNewMusic(musicTrack);
	}
}


void AAudioManager::ReconcileNewMusic(FMusicTrack musicTrack)
{
	USoundBase* Sound = nullptr;

	if (musicTrack.SoundHard) {
		Sound = musicTrack.SoundHard;
		if (DEBUG_MUSIC) {
			FString msg = "Beginning eval of Music: " + Sound->GetName();
			DebugPrintMusicSystem(msg, true);
		}
		// Already loaded sound, just play and have fun
		PlayNewMusic(musicTrack, Sound);
	}
	else if (musicTrack.SoundSoft.IsNull() == false) {
		if (DEBUG_MUSIC) {
			FString msg = "About to load Music: " + musicTrack.SoundSoft->GetPathName();
			DebugPrintMusicSystem(msg, true);
		}
		FSoftObjectPath path = musicTrack.SoundSoft.ToSoftObjectPath();

		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

		Streamable.RequestAsyncLoad(
			path,
			FStreamableDelegate::CreateLambda([this, musicTrack]() {
				if (!musicTrack.SoundSoft.IsNull())	{
					this->PlayNewMusic(musicTrack, musicTrack.SoundSoft.Get());
				}
			})
		);
	}
}


void AAudioManager::PlayNewMusic(FMusicTrack Track, USoundBase* LoadedSound)
{
	if (ensure(LoadedSound)) {
		if (CurrentlyPlayingMusic.SoundHard != LoadedSound)	{
			if (CurrentlyPlayingMusic.instance)	{
				float fadeTime = CurrentlyPlayingMusic.PreviousTrackFadeOut;
				EAudioFaderCurve outCurve = CurrentlyPlayingMusic.FadeOutType;
				CurrentlyPlayingMusic.instance->FadeOut(fadeTime, 0.f, outCurve);
			}

			float fadeInTime = Track.NewTrackFadeIn;
			EAudioFaderCurve curve = Track.FadeInType;
			float vol = Track.Volume;

			UAudioComponent* instance = UGameplayStatics::CreateSound2D(GetWorld(), LoadedSound);
			instance->FadeIn(fadeInTime, vol, 0.f, curve);

			CurrentlyPlayingMusic = Track;
			CurrentlyPlayingMusic.instance = instance;
			if (CurrentlyPlayingMusic.SoundHard == nullptr)	{
				CurrentlyPlayingMusic.SoundHard = LoadedSound;
			}

			if (DEBUG_MUSIC) {
				FString msg = "Played Music: " + LoadedSound->GetName();
				DebugPrintMusicSystem(msg, true);
			}
		}
		else {
			if (DEBUG_MUSIC) {
				FString msg = LoadedSound->GetName() + " was already playing";
				DebugPrintMusicSystem(msg, true);
			}
		}
	}
}
/*
void AAudioManager::StartQuartzClock()
{

	if (!ensure(CurrentlyPlayingMusic->instance))
		return;

	FQuartzTimeSignature timeSig = FQuartzTimeSignature();
	timeSig.NumBeats = CurrentlyPlayingMusic->TimeSignatureBeats;
	timeSig.BeatType = CurrentlyPlayingMusic->TimeSigDemoninator;

	UWorld* world = GetWorld();
	UQuartzSubsystem* qz = UQuartzSubsystem::Get(world);
	FQuartzClockSettings clockSettings = { timeSig, false };
	bool overrideIfExists = true;
	MusicClockHandle = qz->CreateNewClock(world, QuartzMusicClock, clockSettings, overrideIfExists, true);

	MusicClockHandle->StartClock(world, MusicClockHandle);


	//OnMusicQuartzClockBegun(); // hooks into blueprints here if wanted
}
*/

void AAudioManager::StopMusic(float fadeTime, EAudioFaderCurve curveType)
{
	UAudioComponent* instance = CurrentlyPlayingMusic.instance;
	if (instance && instance->IsPlaying()) {
		instance->FadeOut(fadeTime, 0.f, curveType);
	}

	CurrentlyPlayingMusic.SoundHard = nullptr;

	if (DEBUG_MUSIC) {
		FString msg = "Music Stopped";
		DebugPrintMusicSystem(msg, true);
	}
}

void AAudioManager::EvaluateAmbienceMap(ETag tag)
{
	if (AmbienceMap.Find(tag)) {
		LoadNewAmbience(AmbienceMap[tag]);
	}
}

void AAudioManager::LoadNewAmbience(F_2DAmbience ambience)
{
	if (CurrentlyPlayingAmbience.Sound == ambience.Sound) {
		return;
	}

	if (ambience.Sound.IsNull()) {
		return;
	}

	USoundBase* SoundToPlay = nullptr;
	FSoftObjectPath path = ambience.Sound.ToSoftObjectPath();
	
	FStreamableManager& StreamManager = UAssetManager::GetStreamableManager();
	StreamManager.RequestAsyncLoad(
		path,
		FStreamableDelegate::CreateLambda([this, ambience]() {
			if (ambience.Sound.IsValid()) {
				this->PlayNewAmbience(ambience, ambience.Sound.Get());
			}
		})
	);

	if (ambience.IntermittentElements) {
		// Intermittent Elements are optional and might be left blank
		FSoftObjectPath intElementsPath = ambience.IntermittentElements.ToSoftObjectPath();

		StreamManager.RequestAsyncLoad(
			intElementsPath,
			FStreamableDelegate::CreateLambda([this, &ambience]() {
				if (!ambience.IntermittentElements.IsNull()) {
					ambience.LoadedIntElement = ambience.IntermittentElements.Get();
					this->ResetTimerForIntermittentAmbienceElements();
				}
			})
		);
	}
}

void AAudioManager::PlayNewAmbience(F_2DAmbience ambience, USoundBase* Sound)
{
	if (CurrentlyPlayingAmbience.instance) {
		float fadeOutTime = CurrentlyPlayingAmbience.FadeOutTime;
		EAudioFaderCurve curve = CurrentlyPlayingAmbience.FadeOutType;
		CurrentlyPlayingAmbience.instance->FadeOut(fadeOutTime, 0.f, curve);
	}

	if (ensure(Sound)) {
		float fadeIn = ambience.FadeInTime;
		EAudioFaderCurve curve = ambience.FadeInType;
		float vol = ambience.Volume;

		UAudioComponent* instance = UGameplayStatics::CreateSound2D(GetWorld(), Sound, vol, 1.f, 0.f);
		if (instance) {
			instance->FadeIn(fadeIn, vol, 0.f, curve);
			CurrentlyPlayingAmbience = ambience;
			CurrentlyPlayingAmbience.instance = instance;
		}
	}
}

F_2DAmbience& AAudioManager::GetCurrentlyPlayingAmbience()
{
	return CurrentlyPlayingAmbience;
}

void AAudioManager::StopAllAmbience(float fadeTime, EAudioFaderCurve curve)
{
	if (CurrentlyPlayingAmbience.instance) {
		if (CurrentlyPlayingAmbience.instance->IsPlaying())	{
			CurrentlyPlayingAmbience.instance->FadeOut(fadeTime, 0.f, curve);
		}
	}
	CurrentlyPlayingAmbience.Sound = nullptr;
	CurrentlyPlayingAmbience.IntermittentElements = nullptr;
}

///// DEBUG PRINTING /////

void AAudioManager::DebugPrintDxSystem(FString msg, bool cacheMe)
{
	FString m = "[DIALOGUE] " + msg;
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.5f,
			FColor::White,
			m
		);
	}

	if (cacheMe) {
		DebugCachedMsg = m;
		CachedMsgTime = CachedMsgMaxTime;
	}
}

void AAudioManager::DebugPrintDialogueQueue(bool preventActive)
{
	FString msg = "";
	if (CachedMsgTime > 0.f) {
		msg = DebugCachedMsg;
	}
	else {
		msg = TEXT("Queue Size: ");
		msg.Append(FString::FromInt(DialogueQueue.Num()));
		if (preventActive) {
			msg += " - PreventDialogue active";
		}
	}
	DebugPrintDxSystem(msg, false);
}

void AAudioManager::DebugPrintMusicSystem(FString msg, bool GameTime)
{
	FString m = "[MUSIC] " + msg;

	if (GameTime) {
		float gt = GetWorld()->GetTimeSeconds();
		m = m + " at game time: " + (FString::SanitizeFloat(gt, 2));
	}

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Purple,
			m
		);
	}
}