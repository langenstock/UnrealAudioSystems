// Fill out your copyright notice in the Description page of Project Settings.


#include "AudioSubsystem.h"


void UAudioSubsystem::Tick(float DeltaTime)
{
	if (!GetWorld()) { return; };

	if (m_DialogueQueue.IsEmpty() == false) {
		if (m_DEBUG_DIALOGUE) {
			DebugPrintDialogueQueue((m_PreventDialogue > 0.f));
			if (m_CachedMsgTime > 0.f)
			{
				m_CachedMsgTime -= DeltaTime;
			}
		}
		ProcessDialogueQueue();
	}

	if (m_PreventDialogue > 0.f) {
		m_PreventDialogue -= DeltaTime;
	}

	ProcessDialogueDelayQueue(DeltaTime);

	if (m_CurrentlyPlayingAmbience.IntermittentElements) {
		m_CurrentlyPlayingAmbience.timeUntilNextIntermittent -= DeltaTime;
		if (m_CurrentlyPlayingAmbience.timeUntilNextIntermittent < 0.f) {
			AttemptIntermittentAmbSound();
			ResetTimerForIntermittentAmbienceElements();
		}
	}
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


void UAudioSubsystem::InitAudioData(TMap<ETag, FMusicTrack> MusicMap,
									TMap<ETag, F_2DAmbience> AmbienceMap,
									USoundConcurrency* DialogueConcurrency,
									USoundAttenuation* DialogueAttenuation)
{
	m_DxConcurrency = DialogueConcurrency;
	m_DxAttenuation = DialogueAttenuation;
	m_MusicMap = MusicMap;
	m_AmbienceMap = AmbienceMap;
	m_DialogueHistory.Empty(200);
}


void UAudioSubsystem::ResetTimerForIntermittentAmbienceElements()
{
	float r = FMath::FRandRange(m_CurrentlyPlayingAmbience.TriggerTimeMinimum, m_CurrentlyPlayingAmbience.TriggerTimeMaximum);
	m_CurrentlyPlayingAmbience.timeUntilNextIntermittent = r;
}


void UAudioSubsystem::AttemptIntermittentAmbSound()
{
	// Not a typo - we only play intermittents if there is no dialogue playing
	if (m_CurrentlyPlayingDialogue && m_CurrentlyPlayingDialogue->IsPlaying())
		return;

	UAudioComponent* s = UGameplayStatics::SpawnSound2D(GetWorld(), m_CurrentlyPlayingAmbience.LoadedIntElement, 1.f, 1.f);
	if (s) {
		m_CurrentlyPlayingAmbience.intElementsInstance = s;
	}
}




void UAudioSubsystem::ProcessDialogueDelayQueue(float DeltaTime)
{
	TArray<int> indicesToMove;
	size_t j = m_DialogueDelayQueue.Num();

	for (size_t i = 0; i < j; i++) {
		FDialogueLine& line = m_DialogueDelayQueue[i];
		line.TimeSpentInQueue += DeltaTime;
		if (line.TimeSpentInQueue > line.PlayDelay) {
			indicesToMove.Push(i);
		}
	}

	for (int& i : indicesToMove) {
		m_DialogueQueue.Push(m_DialogueDelayQueue[i]);
		m_DialogueDelayQueue.RemoveAt(i);
	}
}

void UAudioSubsystem::ProcessDialogueQueue()
{
	if (m_PreventDialogue > 0.f)
		return;

	float gameTimeNow = (float)GetWorld()->GetTimeSeconds();

	FDialogueLine thisLine = m_DialogueQueue[0];
	bool isGroup = thisLine.isGroup;
	int groupId = thisLine.groupId;
	ECharacters thisLineChar = thisLine.EChar;

	if (IsLineValidAccordingToHistory(thisLine, gameTimeNow) == false) {
		RemoveLineAndItsSequencePartners(0, isGroup, groupId);
		return;
	}

	// if MaxTime is -1 we ignore this
	if (thisLine.MaxTimeCanBeInQueue > 0.f) {
		if (gameTimeNow > (thisLine.GameTimeQueued + thisLine.MaxTimeCanBeInQueue)) {
			if (m_DEBUG_DIALOGUE) {
				FString msg = thisLine.Sound->GetName();
				msg += " was not queued (max time in queue passed)";
				DebugPrintDxSystem(msg, true);
			}
			RemoveLineAndItsSequencePartners(0, isGroup, groupId);
			return;
		}
	}

	if (m_CharacterPreventMap.Find(thisLineChar)) {
		// is game time free greater than game time now?
		if (m_CharacterPreventMap[thisLineChar] > gameTimeNow) {
			if (m_DEBUG_DIALOGUE) {
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

	// TODO DL this should be more general - allow the sound to have it sown att 
	// TODO DL to support worldized dialogue in an open world, maybe have a 'dont play if out of range'
	USoundAttenuation* att = thisLine.AllowReverb ? m_DxAttenuation : nullptr;

	USoundBase* Sound = thisLine.Sound;
	UAudioComponent* LinePlaying = UGameplayStatics::SpawnSoundAtLocation(this, Sound, loc, r, vol,
		pitch, time, att,
		m_DxConcurrency);
	if (LinePlaying && LinePlaying->IsPlaying()) {
		m_CurrentlyPlayingDialogue = LinePlaying;
		thisLine.instance = LinePlaying;

		float lineDuration = Sound->GetDuration();
		float gap = thisLine.Gap;
		if (gap == 0.f)
			gap = m_DefaultGapBetweenLines;

		m_DialogueQueue.RemoveAt(0); // removing because it was successful

		SetPreventDialogue(lineDuration + gap);

		if (m_DialogueHistory.Find(Sound) == nullptr) {
			m_DialogueHistory.Add(TTuple<USoundBase*, FDialogueLine>(Sound, thisLine));
		}
		m_DialogueHistory[Sound].GameTimePlayed = gameTimeNow;
		m_DialogueHistory[Sound].TimesPlayed++;

		AddLineToLastLineFromCharacterMap(thisLine);
	}
}


void UAudioSubsystem::RemoveLineAndItsSequencePartners(int queuePos, bool isGroup, int groupId)
{
	if (isGroup) {
		int size = m_DialogueQueue.Num();
		for (int i = size - 1; i >= 0; i--) {
			if (m_DialogueQueue[i].groupId == groupId) {
				m_DialogueQueue.RemoveAt(i);
			}
		}

		size = m_DialogueDelayQueue.Num();
		for (int i = size - 1; i >= 0; i--) {
			if (m_DialogueDelayQueue[i].groupId == groupId) {
				m_DialogueDelayQueue.RemoveAt(i);
			}
		}
	}
	else {
		m_DialogueQueue.RemoveAt(queuePos);
	}
}

void UAudioSubsystem::RemoveLineAndPartnersFromDelayQueue(int queuePos, bool isGroup, int groupId)
{
	if (isGroup) {
		int size = m_DialogueDelayQueue.Num();
		for (int i = size - 1; i >= 0; i--) {
			if (m_DialogueDelayQueue[i].groupId == groupId) {
				m_DialogueDelayQueue.RemoveAt(i);
			}
		}
	}
	else {
		m_DialogueDelayQueue.RemoveAt(queuePos);
	}
}

bool UAudioSubsystem::IsLineValidAccordingToHistory(const FDialogueLine& Line, float gameTimeNow)
{
	if (m_DialogueHistory.Find(Line.Sound)) {
		FDialogueLine& LineInHistory = m_DialogueHistory[Line.Sound];
		if (LineInHistory.InfiniteCooldown) {
			if (m_DEBUG_DIALOGUE) {
				FString msg = Line.Sound->GetName();
				msg += " was not queued (infinite cooldown)";
				DebugPrintDxSystem(msg, true);
			}
			return false;
		}
		else if (LineInHistory.Cooldown > (gameTimeNow - LineInHistory.GameTimePlayed)) {
			if (m_DEBUG_DIALOGUE) {
				FString msg = Line.Sound->GetName();
				msg += " was not queued (still on cooldown)";
				DebugPrintDxSystem(msg, true);
			}
			return false;
		}
		if (LineInHistory.MaxTimesCanPlay > 0) { // if this value is -1 there is no limit
			if (LineInHistory.TimesPlayed >= LineInHistory.MaxTimesCanPlay) {
				if (m_DEBUG_DIALOGUE) {
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


void UAudioSubsystem::QueueDialogueLine(FDialogueLine line, EQueueType queueType)
{
	line.GameTimeQueued = (float)GetWorld()->GetTimeSeconds();
	if (line.PlayDelay > 0.f) {
		m_DialogueDelayQueue.Push(line);
	}
	else {
		m_DialogueQueue.Push(line);
	}

	PostQueueTypeAction(queueType);
}


void UAudioSubsystem::PostQueueTypeAction(EQueueType queueType)
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


void UAudioSubsystem::AddLineToLastLineFromCharacterMap(FDialogueLine& Line)
{
	m_LastLineFromCharacterMap.Add({ Line.EChar, Line });
}


void UAudioSubsystem::QueueDialogueSequence(TArray<FDialogueLine> lines, EQueueType queueType)
{
	// Evaluate validity of line 0
	FDialogueLine& LineZero = lines[0];
	FDialogueLine* LineZeroInHistory = m_DialogueHistory.Find(LineZero.Sound);

	if (LineZeroInHistory) {
		if (LineZeroInHistory->InfiniteCooldown == true) {
			if (m_DEBUG_DIALOGUE) {
				FString msg = LineZero.Sound->GetName();
				msg += " was not queued (infinite cooldown)";
				DebugPrintDxSystem(msg, true);
			}
			return;		// i.e. dont bother trying to queue any of this sequence

		}
		else if (LineZeroInHistory->MaxTimesCanPlay > 0) {
			if (LineZeroInHistory->TimesPlayed >= LineZeroInHistory->MaxTimesCanPlay) {
				if (m_DEBUG_DIALOGUE) {
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
	int maxTimesCanPlay = LineZero.MaxTimesCanPlay;

	float runningDuration = playDelay + LineZero.Sound->GetDuration();

	if (LineZero.MaxTimeCanBeInQueue < 0.f) {
		LineZero.MaxTimeCanBeInQueue = runningDuration;
		// otherwise subsequent lines could potentially time out leaving line zero behind
	}

	for (auto& l : lines) {
		if (m_CharacterPreventMap.Find(l.EChar)) {
			if (m_CharacterPreventMap[l.EChar] > (GetWorld()->GetTimeSeconds())) {
				if (m_DEBUG_DIALOGUE) {
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
			float thisLineGap = (l.Gap <= 0.f) ? m_DefaultGapBetweenLines : l.Gap;
			l.PlayDelay = playDelay;
			runningDuration = runningDuration + thisLineDur + thisLineGap;
		}
	}

	int thisGroupId = ++m_NextGroupId;

	for (auto& l : lines) {
		// For line[0] the max time is what is specified, for other lines, it is the calc'd duration of the seq
		if (l.Sound != LineZero.Sound) {
			l.MaxTimeCanBeInQueue = runningDuration;
		}

		l.MaxTimesCanPlay = maxTimesCanPlay;
		l.InfiniteCooldown = infCooldown;
		l.Cooldown = cooldown;
		l.isGroup = true;
		l.groupId = thisGroupId;
		QueueDialogueLine(l, EQueueType::NONE);
	}

	PostQueueTypeAction(queueType);
}

void UAudioSubsystem::SetPreventDialogue(float duration)
{
	m_PreventDialogue = duration;
}

void UAudioSubsystem::InterruptAndClearQueues()
{
	ClearQueues();

	if (m_CurrentlyPlayingDialogue) {
		if (m_CurrentlyPlayingDialogue->IsPlaying()) {
			m_CurrentlyPlayingDialogue->Stop();
		}
	}

	if (m_DEBUG_DIALOGUE) {
		FString msg = "Interrupt and clear queues called";
		DebugPrintDxSystem(msg, true);
	}
}

void UAudioSubsystem::ClearQueues()
{
	m_DialogueQueue.Empty();
	m_DialogueDelayQueue.Empty();
}

void UAudioSubsystem::InterruptCharacter(ECharacters EChar)
{
	bool origLineIsGroup = false;
	int origLineGroupId = 0;
	if (m_LastLineFromCharacterMap.Find(EChar)) {
		FDialogueLine& lineToStop = m_LastLineFromCharacterMap[EChar];
		UAudioComponent* sound = lineToStop.instance;
		if (sound && sound->IsPlaying()) {
			if (lineToStop.isGroup) {
				origLineGroupId = lineToStop.groupId;
				origLineIsGroup = true;
			}
			sound->Stop();
			if (m_DEBUG_DIALOGUE) {
				FString msg = lineToStop.Sound->GetName();
				msg += " was interrupted (interrupt character)";
				DebugPrintDxSystem(msg, true);
			}
		}
	}

	int size = m_DialogueQueue.Num();
	for (int i = size - 1; i >= 0; i--) {
		if (m_DialogueQueue.Num() > i) { // because the below functions may also remove elements
			if (origLineIsGroup) {
				if (m_DialogueQueue[i].isGroup && m_DialogueQueue[i].groupId == origLineGroupId) {
					RemoveLineAndItsSequencePartners(i, origLineIsGroup, origLineGroupId);
				}
			}
			else if (m_DialogueQueue[i].EChar == EChar) {
				bool isGrp = m_DialogueQueue[i].isGroup;
				int grpId = m_DialogueQueue[i].groupId;
				RemoveLineAndItsSequencePartners(i, isGrp, grpId);
			}
		}
	}

	size = m_DialogueDelayQueue.Num();
	for (int i = size - 1; i >= 0; i--) {
		if (m_DialogueDelayQueue.Num() > i) { // because the below functions may also remove elements
			if (m_DialogueDelayQueue[i].EChar == EChar) {
				bool isGroup = m_DialogueDelayQueue[i].isGroup;
				int groupId = m_DialogueDelayQueue[i].groupId;
				RemoveLineAndPartnersFromDelayQueue(i, isGroup, groupId);
			}
		}
	}
}

void UAudioSubsystem::SetPreventCharacterSpeech(ECharacters EChar, float time)
{
	float gt = GetWorld()->GetTimeSeconds();
	float gameTimeWillBeFree = gt + time;
	m_CharacterPreventMap.Add({ EChar, gameTimeWillBeFree });
}


FMusicTrack& UAudioSubsystem::GetCurrentlyPlayingMusic()
{
	return m_CurrentlyPlayingMusic;
}


void UAudioSubsystem::EvaluateMusicMap(ETag tag)
{
	if (m_MusicMap.Find(tag)) {
		FMusicTrack& musicTrack = m_MusicMap[tag];
		ReconcileNewMusic(musicTrack);
	}
}


void UAudioSubsystem::ReconcileNewMusic(FMusicTrack musicTrack)
{
	USoundBase* Sound = nullptr;

	if (musicTrack.SoundHard) {
		Sound = musicTrack.SoundHard;
		if (m_DEBUG_MUSIC) {
			FString msg = "Beginning eval of Music: " + Sound->GetName();
			DebugPrintMusicSystem(msg, true);
		}
		// Already loaded sound, just play and have fun
		PlayNewMusic(musicTrack, Sound);
	}
	else if (musicTrack.SoundSoft.IsNull() == false) {
		if (m_DEBUG_MUSIC) {
			FString msg = "About to load Music: " + musicTrack.SoundSoft->GetPathName();
			DebugPrintMusicSystem(msg, true);
		}
		FSoftObjectPath path = musicTrack.SoundSoft.ToSoftObjectPath();

		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

		Streamable.RequestAsyncLoad(
			path,
			FStreamableDelegate::CreateLambda([this, musicTrack]() {
				if (!musicTrack.SoundSoft.IsNull()) {
					this->PlayNewMusic(musicTrack, musicTrack.SoundSoft.Get());
				}
				})
		);
	}
}


void UAudioSubsystem::PlayNewMusic(FMusicTrack Track, USoundBase* LoadedSound)
{
	if (ensure(LoadedSound)) {
		if (m_CurrentlyPlayingMusic.SoundHard != LoadedSound) {
			if (m_CurrentlyPlayingMusic.instance) {
				float fadeTime = m_CurrentlyPlayingMusic.PreviousTrackFadeOut;
				EAudioFaderCurve outCurve = m_CurrentlyPlayingMusic.FadeOutType;
				m_CurrentlyPlayingMusic.instance->FadeOut(fadeTime, 0.f, outCurve);
			}

			float fadeInTime = Track.NewTrackFadeIn;
			EAudioFaderCurve curve = Track.FadeInType;
			float vol = Track.Volume;

			UAudioComponent* instance = UGameplayStatics::CreateSound2D(GetWorld(), LoadedSound);
			instance->FadeIn(fadeInTime, vol, 0.f, curve);

			m_CurrentlyPlayingMusic = Track;
			m_CurrentlyPlayingMusic.instance = instance;
			if (m_CurrentlyPlayingMusic.SoundHard == nullptr) {
				m_CurrentlyPlayingMusic.SoundHard = LoadedSound;
			}

			if (m_DEBUG_MUSIC) {
				FString msg = "Played Music: " + LoadedSound->GetName();
				DebugPrintMusicSystem(msg, true);
			}
		}
		else {
			if (m_DEBUG_MUSIC) {
				FString msg = LoadedSound->GetName() + " was already playing";
				DebugPrintMusicSystem(msg, true);
			}
		}
	}
}


void UAudioSubsystem::StopMusic(float fadeTime, EAudioFaderCurve curveType)
{
	UAudioComponent* instance = m_CurrentlyPlayingMusic.instance;
	if (instance && instance->IsPlaying()) {
		instance->FadeOut(fadeTime, 0.f, curveType);
	}

	m_CurrentlyPlayingMusic.SoundHard = nullptr;

	if (m_DEBUG_MUSIC) {
		FString msg = "Music Stopped";
		DebugPrintMusicSystem(msg, true);
	}
}

void UAudioSubsystem::EvaluateAmbienceMap(ETag tag)
{
	if (m_AmbienceMap.Find(tag)) {
		LoadNewAmbience(m_AmbienceMap[tag]);
	}
}

void UAudioSubsystem::LoadNewAmbience(F_2DAmbience ambience)
{
	if (m_CurrentlyPlayingAmbience.Sound == ambience.Sound) {
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

void UAudioSubsystem::PlayNewAmbience(F_2DAmbience ambience, USoundBase* Sound)
{
	if (m_CurrentlyPlayingAmbience.instance) {
		float fadeOutTime = m_CurrentlyPlayingAmbience.FadeOutTime;
		EAudioFaderCurve curve = m_CurrentlyPlayingAmbience.FadeOutType;
		m_CurrentlyPlayingAmbience.instance->FadeOut(fadeOutTime, 0.f, curve);
	}

	if (ensure(Sound)) {
		float fadeIn = ambience.FadeInTime;
		EAudioFaderCurve curve = ambience.FadeInType;
		float vol = ambience.Volume;

		UAudioComponent* instance = UGameplayStatics::CreateSound2D(GetWorld(), Sound, vol, 1.f, 0.f);
		if (instance) {
			instance->FadeIn(fadeIn, vol, 0.f, curve);
			m_CurrentlyPlayingAmbience = ambience;
			m_CurrentlyPlayingAmbience.instance = instance;
		}
	}
}

F_2DAmbience& UAudioSubsystem::GetCurrentlyPlayingAmbience()
{
	return m_CurrentlyPlayingAmbience;
}

void UAudioSubsystem::StopAllAmbience(float fadeTime, EAudioFaderCurve curve)
{
	if (m_CurrentlyPlayingAmbience.instance) {
		if (m_CurrentlyPlayingAmbience.instance->IsPlaying()) {
			m_CurrentlyPlayingAmbience.instance->FadeOut(fadeTime, 0.f, curve);
		}
	}
	m_CurrentlyPlayingAmbience.Sound = nullptr;
	m_CurrentlyPlayingAmbience.IntermittentElements = nullptr;
}

///// DEBUG PRINTING /////

void UAudioSubsystem::DebugPrintDxSystem(FString msg, bool cacheMe)
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
		m_DebugCachedMsg = m;
		m_CachedMsgTime = m_CachedMsgMaxTime;
	}
}

void UAudioSubsystem::DebugPrintDialogueQueue(bool preventActive)
{
	FString msg = "";
	if (m_CachedMsgTime > 0.f) {
		msg = m_DebugCachedMsg;
	}
	else {
		msg = TEXT("Queue Size: ");
		msg.Append(FString::FromInt(m_DialogueQueue.Num()));
		if (preventActive) {
			msg += " - PreventDialogue active";
		}
	}
	DebugPrintDxSystem(msg, false);
}

void UAudioSubsystem::DebugPrintMusicSystem(FString msg, bool GameTime)
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