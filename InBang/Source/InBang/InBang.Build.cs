using UnrealBuildTool;

public class InBang : ModuleRules
{
	public InBang(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "CinematicCamera", "SlateCore", "RigLogicModule" });
	}
}
