using UnrealBuildTool;
public class InjuryLabEditor : ModuleRules
{
    public InjuryLabEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InjuryLab", "UnrealEd", "UMG", "UMGEditor", "Slate", "SlateCore", "Kismet", "AssetRegistry" });
    }
}
