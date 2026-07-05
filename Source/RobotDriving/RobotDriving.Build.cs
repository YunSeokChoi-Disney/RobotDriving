using UnrealBuildTool;

public class RobotDriving : ModuleRules
{
    public RobotDriving(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        bEnableExceptions = true;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Sockets", "Networking" });
        PrivateDependencyModuleNames.AddRange(new string[] { });

        
    }
   
}