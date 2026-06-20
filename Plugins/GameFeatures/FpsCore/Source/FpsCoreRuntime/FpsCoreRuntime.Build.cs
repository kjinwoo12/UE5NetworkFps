// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FpsCoreRuntime : ModuleRules
{
	public FpsCoreRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"LyraGame",
				"ModularGameplay",
				"CommonGame",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"GameplayAbilities",
				"EnhancedInput",
			}
		);
	}
}
