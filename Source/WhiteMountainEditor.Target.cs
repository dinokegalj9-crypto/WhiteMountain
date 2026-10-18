using UnrealBuildTool;

public class WhiteMountainEditorTarget : TargetRules
{
	public WhiteMountainEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("WhiteMountain");
	}
}
