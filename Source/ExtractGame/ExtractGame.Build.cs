// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ExtractGame : ModuleRules
{
	public ExtractGame(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ExtractGame",
			"ExtractGame/Variant_Platforming",
			"ExtractGame/Variant_Platforming/Animation",
			"ExtractGame/Variant_Combat",
			"ExtractGame/Variant_Combat/AI",
			"ExtractGame/Variant_Combat/Animation",
			"ExtractGame/Variant_Combat/Gameplay",
			"ExtractGame/Variant_Combat/Interfaces",
			"ExtractGame/Variant_Combat/UI",
			"ExtractGame/Variant_SideScrolling",
			"ExtractGame/Variant_SideScrolling/AI",
			"ExtractGame/Variant_SideScrolling/Gameplay",
			"ExtractGame/Variant_SideScrolling/Interfaces",
			"ExtractGame/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
