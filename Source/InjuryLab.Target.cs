using UnrealBuildTool;
public class InjuryLabTarget : TargetRules
{
    public InjuryLabTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("InjuryLab");
    }
}
