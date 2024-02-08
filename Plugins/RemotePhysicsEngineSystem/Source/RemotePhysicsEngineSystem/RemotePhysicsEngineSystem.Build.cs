// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RemotePhysicsEngineSystem : ModuleRules
{
	public RemotePhysicsEngineSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
			
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
            "Networking",
            "Sockets"
        });
			
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore"
		});
	}
}
