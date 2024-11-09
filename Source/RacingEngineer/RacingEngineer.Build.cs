// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RacingEngineer : ModuleRules
{
	public RacingEngineer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ChaosVehicles", "PhysicsCore", "Landscape"});

		PrivateDependencyModuleNames.AddRange( new string[] { "ProceduralMeshComponent", "RenderCore", "RHI" });
	}
}
