using UnrealBuildTool;

public class InBang : ModuleRules
{
	public InBang(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		// CinematicCamera: 방송 카메라, SlateCore: 아바타 캡처 창 제목 설정
		PrivateDependencyModuleNames.AddRange(new string[] { "CinematicCamera", "SlateCore" });
	}
}
