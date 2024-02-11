// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MultiplayerPhaaS : ModuleRules
{
	public MultiplayerPhaaS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;
        CppStandard = CppStandardVersion.Cpp17;

        PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"UMG",
			"OnlineSubsystem",
            "OnlineSubsystemSteam",
			"RemotePhysicsEngineSystem",
            "LocalPhysicsEngineSystem"
        });

		PrivateDependencyModuleNames.AddRange(new string[] 
		{  

		});
	}
}
