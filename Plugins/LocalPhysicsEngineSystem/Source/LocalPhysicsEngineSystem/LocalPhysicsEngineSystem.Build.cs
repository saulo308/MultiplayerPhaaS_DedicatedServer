// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LocalPhysicsEngineSystem : ModuleRules
{
	public LocalPhysicsEngineSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;
        CppStandard = CppStandardVersion.Cpp17;
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"JoltPhysicsWrapper"
		});
	}
}
