// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AudioSystems : ModuleRules
{
	public AudioSystems(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"MetasoundEngine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"AudioSystems",
			"AudioSystems/Variant_Horror",
			"AudioSystems/Variant_Horror/UI",
			"AudioSystems/Variant_Shooter",
			"AudioSystems/Variant_Shooter/AI",
			"AudioSystems/Variant_Shooter/UI",
			"AudioSystems/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
