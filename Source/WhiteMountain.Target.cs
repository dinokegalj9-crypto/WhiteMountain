using UnrealBuildTool;

public class WhiteMountainTarget : TargetRules
{
	public WhiteMountainTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("WhiteMountain");
	}
}
