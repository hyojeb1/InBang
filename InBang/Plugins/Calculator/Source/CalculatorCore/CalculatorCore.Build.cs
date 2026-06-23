using UnrealBuildTool;

public class CalculatorCore : ModuleRules
{
	public CalculatorCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject"
		});
	}
}
