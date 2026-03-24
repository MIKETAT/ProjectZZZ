// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectZZZ : ModuleRules
{
	public ProjectZZZ(ReadOnlyTargetRules Target) : base(Target)
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
			"ProjectZZZ",
			"ProjectZZZ/Variant_Platforming",
			"ProjectZZZ/Variant_Platforming/Animation",
			"ProjectZZZ/Variant_Combat",
			"ProjectZZZ/Variant_Combat/AI",
			"ProjectZZZ/Variant_Combat/Animation",
			"ProjectZZZ/Variant_Combat/Gameplay",
			"ProjectZZZ/Variant_Combat/Interfaces",
			"ProjectZZZ/Variant_Combat/UI",
			"ProjectZZZ/Variant_SideScrolling",
			"ProjectZZZ/Variant_SideScrolling/AI",
			"ProjectZZZ/Variant_SideScrolling/Gameplay",
			"ProjectZZZ/Variant_SideScrolling/Interfaces",
			"ProjectZZZ/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
