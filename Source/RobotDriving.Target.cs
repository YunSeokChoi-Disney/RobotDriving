using UnrealBuildTool;
using System.Collections.Generic;

public class RobotDrivingTarget : TargetRules
{
    public RobotDrivingTarget(TargetInfo Target) : base(Target)
    {
       
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("RobotDriving");
    }
}