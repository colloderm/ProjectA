// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectA : ModuleRules
{
	public ProjectA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", 
			"GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG", "DeveloperSettings"
		});

		// Slate UI를 사용하면 주석 해제
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		PublicIncludePaths.AddRange(new string[] {
			"ProjectA",
			"ProjectA/Combat",
			"ProjectA/Enemy",
			"ProjectA/GAS",
			"ProjectA/GAS/Abilities",
			"ProjectA/GAS/Effects",
			"ProjectA/Guild",
			"ProjectA/Hero",
			"ProjectA/Pickup",
			"ProjectA/Run",
			"ProjectA/Skills",
			"ProjectA/UI",
		});
		
		// 온라인 기능을 사용하면 주석 해제
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// OnlineSubsystemSteam을 쓰려면 uproject 파일의 plugins 항목에 Enabled = true로 추가
	}
}
