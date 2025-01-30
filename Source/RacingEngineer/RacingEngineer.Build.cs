// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RacingEngineer : ModuleRules
{
	public RacingEngineer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ChaosVehicles", "PhysicsCore", "Landscape", "SlateCore", "Foliage" });

		PrivateDependencyModuleNames.AddRange( new string[] { "ProceduralMeshComponent", "RenderCore", "RHI" });

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			{
				PublicAdditionalLibraries.AddRange(new string[] {
					"shlwapi.lib",
					"propsys.lib",
					"comctl32.lib"
				});

				PublicDelayLoadDLLs.AddRange(new string[] {
					"shlwapi.dll",
					"propsys.dll",
					"comctl32.dll"
				});

				PublicDefinitions.Add("STRICT_TYPED_ITEMIDS=1");
			}
        }
	}
}
