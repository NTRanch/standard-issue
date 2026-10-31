// Build rules — PREFLIGHT-VERIFY against the UE 5.8 install on primo.
using UnrealBuildTool;

public class StandardIssue : ModuleRules
{
    public StandardIssue(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
    }
}
