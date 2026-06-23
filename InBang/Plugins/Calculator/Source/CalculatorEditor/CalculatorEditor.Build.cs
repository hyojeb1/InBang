using UnrealBuildTool;

public class CalculatorEditor : ModuleRules
{
	public CalculatorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd",
			"CalculatorCore",
			"CalculatorRuntime"
		});
	}
}
