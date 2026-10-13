using UnrealBuildTool;
public class InjuryLabEditorTarget : TargetRules
{
    public InjuryLabEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.AddRange(new string[] { "InjuryLab", "InjuryLabEditor" });
    }
}
