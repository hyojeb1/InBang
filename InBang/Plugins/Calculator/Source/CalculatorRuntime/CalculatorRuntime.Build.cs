using UnrealBuildTool;

public class CalculatorRuntime : ModuleRules
{
	public CalculatorRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CalculatorCore"
		});
	}
}
