// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGuildSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Guild/MLGuildSettings.h"
#include "Guild/MLSaveGame.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* SaveSlotName = TEXT("Guild");
	constexpr int32 SaveUserIndex = 0;

	// 웹 db.js NAME_POOL. shortcut: 일반 문자열, 현지화할 때 String Table로 이동 (design §11)
	const TCHAR* MercNames[] =
	{
		TEXT("카이"), TEXT("레온"), TEXT("아리아"), TEXT("바이런"), TEXT("셀린"), TEXT("드레이크"), TEXT("에밀"), TEXT("피오나"),
		TEXT("가레스"), TEXT("하넬"), TEXT("이반"), TEXT("주노"), TEXT("카일"), TEXT("리나"), TEXT("마커스"), TEXT("노아"),
		TEXT("오스카"), TEXT("페트라"), TEXT("퀸"), TEXT("로한"), TEXT("실비아"), TEXT("타이런"), TEXT("울릭"), TEXT("베라"),
		TEXT("윈터"), TEXT("제로"), TEXT("아셀"), TEXT("브릭"), TEXT("코델"), TEXT("단테"), TEXT("엘드릭"), TEXT("판가"),
		TEXT("글렌"), TEXT("하운드"), TEXT("이그니스"), TEXT("잭"), TEXT("크라운"), TEXT("루카"), TEXT("모건"), TEXT("나이트"),
	};

	FString GradeName(EMLGrade Grade)
	{
		return StaticEnum<EMLGrade>()->GetNameStringByValue(static_cast<int64>(Grade));
	}

	bool ParseGrade(const FString& Text, EMLGrade& OutGrade)
	{
		for (int32 g = 0; g < static_cast<int32>(EMLGrade::Count); ++g)
		{
			if (GradeName(static_cast<EMLGrade>(g)).Equals(Text, ESearchCase::IgnoreCase))
			{
				OutGrade = static_cast<EMLGrade>(g);
				return true;
			}
		}
		return false;
	}

	FString StatsToString(const FMLStatBlock& S)
	{
		return FString::Printf(TEXT("hp=%.0f atk=%.0f def=%.0f spd=%.0f crit=%.0f dodge=%.0f special=%.0f"),
			S.HP, S.ATK, S.DEF, S.SPD, S.CRIT, S.DODGE, S.SPECIAL);
	}
	
	FString WeaponName(EMLAttackType Type)
	{
		return StaticEnum<EMLAttackType>()->GetNameStringByValue(static_cast<int64>(Type)); // = DT_Weapons 행 이름
	}

	FString SlotName(EMLItemSlot Slot)
	{
		return StaticEnum<EMLItemSlot>()->GetNameStringByValue(static_cast<int64>(Slot));
	}

	bool ParseItemSlot(const FString& Text, EMLItemSlot& OutSlot)
	{
		for (int32 s = 0; s < static_cast<int32>(EMLItemSlot::Count); ++s)
		{
			if (SlotName(static_cast<EMLItemSlot>(s)).Equals(Text, ESearchCase::IgnoreCase))
			{
				OutSlot = static_cast<EMLItemSlot>(s);
				return true;
			}
		}
		return false;
	}

	UMLGuildSubsystem* GetGuild(const UWorld* World)
	{
		const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UMLGuildSubsystem>() : nullptr;
	}
	
	int32 RequestProgress(const FMLRequestRow& Request, const FMLRunReport& Report)
	{
		switch (Request.Type)
		{
		case EMLRequestType::ReachZone:  return Report.Zone;
		case EMLRequestType::Kills:      return Report.Kills;
		case EMLRequestType::EliteKills: return Report.EliteKills;
		case EMLRequestType::BossKill:   return Report.bBossKilled ? 1 : 0;
		case EMLRequestType::GoldRooms:  return Report.GoldRoomsCleared;
		case EMLRequestType::EliteRooms: return Report.EliteRoomsCleared;
		default:                         return Report.Greed;
		}
	}

	// 개발 콘솔 (M8 길드 UI 전까지): PIE에서 ` 키
	FAutoConsoleCommandWithWorldAndArgs GuildStatusCmd(TEXT("ML.Guild.Status"), TEXT("Log gold, merc, candidates, hall of fame"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (const UMLGuildSubsystem* Guild = GetGuild(World)) Guild->LogStatus();
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildContractCmd(TEXT("ML.Guild.Contract"), TEXT("ML.Guild.Contract <F..S> <Index>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMLGuildSubsystem* Guild = GetGuild(World);
			EMLGrade Grade = EMLGrade::F;
			if (!Guild || Args.Num() < 1 || !ParseGrade(Args[0], Grade))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: usage ML.Guild.Contract <F..S> <Index>"));
				return;
			}
			Guild->Contract(Grade, Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0);
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildAddGoldCmd(TEXT("ML.Guild.AddGold"), TEXT("ML.Guild.AddGold <Amount>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World)) Guild->AddGold(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1000);
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildResetCmd(TEXT("ML.Guild.Reset"), TEXT("Delete the guild save and start fresh"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World)) Guild->ResetSave();
		}));
	
		FAutoConsoleCommandWithWorldAndArgs GuildRetireCmd(TEXT("ML.Guild.Retire"), TEXT("ML.Guild.Retire <TalentIndex> [ReplaceSlot]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World))
			{
				Guild->Retire(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0, Args.Num() > 1 ? FCString::Atoi(*Args[1]) : INDEX_NONE);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildSellCmd(TEXT("ML.Guild.Sell"), TEXT("Sell the current merc for gold"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World)) Guild->Sell();
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildTrainCmd(TEXT("ML.Guild.Train"), TEXT("ML.Guild.Train <hp|atk|def|spd|crit|dodge|special>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World); Guild && Args.Num() > 0) Guild->Train(FName(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildUpgradeCmd(TEXT("ML.Guild.Upgrade"), TEXT("ML.Guild.Upgrade <atk|def|hp|gold|xp>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World); Guild && Args.Num() > 0) Guild->BuyFamilyUpgrade(FName(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildRerollCmd(TEXT("ML.Guild.Reroll"), TEXT("Redraw every contract candidate for gold"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World)) Guild->RerollCandidates();
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildAddPointsCmd(TEXT("ML.Guild.AddPoints"), TEXT("ML.Guild.AddPoints <Amount> (family points + renown)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World)) Guild->AddPoints(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 5);
		}));
	
	FAutoConsoleCommandWithWorldAndArgs GuildEquipCmd(TEXT("ML.Guild.Equip"), TEXT("ML.Guild.Equip <StashIndex>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UMLGuildSubsystem* Guild = GetGuild(World); Guild && Args.Num() > 0) Guild->Equip(FCString::Atoi(*Args[0]));
	}));

	FAutoConsoleCommandWithWorldAndArgs GuildUnequipCmd(TEXT("ML.Guild.Unequip"), TEXT("ML.Guild.Unequip <weapon|armor|trinket>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMLGuildSubsystem* Guild = GetGuild(World);
			EMLItemSlot Slot = EMLItemSlot::Weapon;
			if (!Guild || Args.Num() < 1 || !ParseItemSlot(Args[0], Slot))
			{
				UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: usage ML.Guild.Unequip <weapon|armor|trinket>"));
				return;
			}
			Guild->Unequip(Slot);
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildSellItemCmd(TEXT("ML.Guild.SellItem"), TEXT("ML.Guild.SellItem <StashIndex>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World); Guild && Args.Num() > 0) Guild->SellItem(FCString::Atoi(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildAddItemCmd(TEXT("ML.Guild.AddItem"), TEXT("ML.Guild.AddItem <DT_Items row>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMLGuildSubsystem* Guild = GetGuild(World); Guild && Args.Num() > 0) Guild->AddItem(FName(*Args[0]));
		}));

	FAutoConsoleCommandWithWorldAndArgs GuildAddGraveCmd(TEXT("ML.Guild.AddGrave"), TEXT("ML.Guild.AddGrave <Zone> <sword|bow|staff> [Skill ...] (copies the current merc's equipment; Skill = BP_Skill_<name>, e.g. Blades)"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMLGuildSubsystem* Guild = GetGuild(World);
			const int64 Type = Args.Num() > 1 ? StaticEnum<EMLAttackType>()->GetValueByNameString(Args[1]) : INDEX_NONE;
			if (!Guild || Type == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: usage ML.Guild.AddGrave <Zone> <sword|bow|staff>"));
				return;
			}
			TArray<FSoftClassPath> Skills;
			for (int32 i = 2; i < Args.Num(); ++i)
			{
				const FSoftClassPath Path(FString::Printf(TEXT("/Game/Blueprints/Skills/BP_Skill_%s.BP_Skill_%s_C"), *Args[i], *Args[i]));
				if (Path.TryLoadClass<UObject>()) Skills.Add(Path);
				else UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: add grave skips unknown skill %s"), *Args[i]);
			}
			Guild->AddGrave(FMath::Max(1, FCString::Atoi(*Args[0])), static_cast<EMLAttackType>(Type), Skills);
		}));
}

void UMLGuildSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// shortcut: config 소프트 참조는 자동 쿡되지 않음; 패키징 전에 DataTable 폴더를 "Additional Asset Directories to Cook"에 추가 (M11)
	const UMLGuildSettings* Settings = GetDefault<UMLGuildSettings>();
	GradeTable			= Settings->GradeTable.LoadSynchronous();
	TalentTable			= Settings->TalentTable.LoadSynchronous();
	FamilyUpgradeTable	= Settings->FamilyUpgradeTable.LoadSynchronous();
	TrainingTable		= Settings->TrainingTable.LoadSynchronous();
	ItemTable			= Settings->ItemTable.LoadSynchronous();
	RequestTable		= Settings->RequestTable.LoadSynchronous(); // 선택 항목이라 아래 경고에 포함하지 않음
	TitleTable			= Settings->TitleTable.LoadSynchronous();   // 선택 항목
	if (!GradeTable || !TalentTable || !FamilyUpgradeTable || !TrainingTable || !ItemTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: set Grade/Talent/FamilyUpgrade/Training/Item Table in Project Settings > Mercenary Legacy"));
	}

	SaveData = Cast<UMLSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	if (!SaveData)
	{
		CreateNewSave();
	}
	else if (SaveData->bRunInProgress)
	{
		// design §6.6 (c): 보상은 원정 종료 때만 적용되므로 저장된 용병은 이미 원정 전 상태.
		SaveData->bRunInProgress = false;
		SaveData->ActiveRequest = NAME_None; // 비정상 종료된 원정의 의뢰는 무효
		bRollbackNotice = true;              // 길드 허브가 안내를 한 번 표시
		Save();
	}
	
	// shortcut: 빈 목록도 여기서 다시 채움 (M8-2b 이전 저장, 또는 다음 실행 시 다 팔린 상점)
	if (SaveData->ShopOffers.Num() == 0)
	{
		RefreshShop();
		Save();
	}
	
	if (SaveData->RequestOffers.Num() == 0)
	{
		RefreshRequests();
		Save();
	}

	// design §8.7: M9-2 이전 저장은 길드 누적 기록이 없으므로 한 번만 이어받음
	if (!SaveData->bGuildStatsSeeded)
	{
		SeedGuildStats();
		Save();
	}
}

void UMLGuildSubsystem::SeedGuildStats()
{
	FMLGuildStats& Stats = SaveData->GuildStats;
	auto AddMerc = [&Stats](const FMLMercData& Merc)
	{
		Stats.Extractions += Merc.Extractions;
		Stats.BossKills += Merc.BossKills;
		Stats.GoldEarned += Merc.GoldEarned;
	};
	for (const FMLHallEntry& Entry : SaveData->HallOfFame)
	{
		AddMerc(Entry.Merc);
		if (Entry.Reason == EMLLeaveReason::Fallen) ++Stats.Deaths;
	}
	if (SaveData->bHasMerc) AddMerc(SaveData->Merc);
	SaveData->bGuildStatsSeeded = true; // 처치·엘리트 처치는 기록이 없어 0부터
}

bool UMLGuildSubsystem::Contract(EMLGrade Grade, int32 Index)
{
	if (!CanAct(TEXT("contract"))) return false;
	if (SaveData->bHasMerc) // design §8.1: 용병은 한 번에 한 명
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: contract refused (already own a merc)"));
		return false;
	}

	const FMLGradeRow* Row = FindGrade(Grade);
	int32 Found = INDEX_NONE;
	int32 Seen = 0;
	for (int32 i = 0; i < SaveData->Candidates.Num(); ++i)
	{
		if (SaveData->Candidates[i].Grade == Grade && Seen++ == Index)
		{
			Found = i;
			break;
		}
	}
	if (!Row || Found == INDEX_NONE || SaveData->Gold < Row->ContractGold)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: contract %s#%d refused (no candidate or gold %d < %d)"),
			*GradeName(Grade), Index, SaveData->Gold, Row ? Row->ContractGold : 0);
		return false;
	}

	SaveData->Gold -= Row->ContractGold;
	SaveData->Merc = SaveData->Candidates[Found];
	SaveData->Candidates.RemoveAt(Found);
	SaveData->bHasMerc = true;
	Save();

	return true;
}

void UMLGuildSubsystem::BeginRun()
{
	// design §11: 용병 없음 = 원정 없음 (AMLRunGameMode::BeginPlay가 플레이어를 길드로 돌려보냄)
	if (!SaveData || !SaveData->bHasMerc) return;

	SaveData->bRunInProgress = true;
	Save();
}

FMLRunReward UMLGuildSubsystem::EndRun(const FMLRunReport& Report)
{
	FMLRunReward Reward;
	if (!SaveData) return Reward;

	SaveData->bRunInProgress = false;
	SaveData->RerollCount = 0; // design §8.1: 다시 뽑기 가격은 원정마다 초기화

	FMLStatBlock FamilyStatPercent;
	float FamilyGoldPercent = 0.f;
	float FamilyXPPercent = 0.f;
	GetFamilyBonuses(FamilyStatPercent, FamilyGoldPercent, FamilyXPPercent);

	// design §8.7: 칭호 골드·경험치 보너스는 가문 보너스에 더함 (용병이 전당으로 가기 전에 읽음)
	if (const FMLTitleRow* Title = SaveData->bHasMerc ? GetMercTitle(SaveData->Merc) : nullptr)
	{
		FamilyGoldPercent += Title->GoldPercent;
		FamilyXPPercent += Title->XPPercent;
	}

	// 새 칭호 알림용: 누적 기록을 바꾸기 전에 해금 목록을 기억
	TArray<FName> UnlockedBefore;
	for (const FName& Id : GetTitleIds())
	{
		if (IsTitleUnlocked(Id)) UnlockedBefore.Add(Id);
	}

	if (Report.bExtracted)
	{
		// design §8.3 가문의 부. shortcut: 용병 등급 골드 보정 (design §8.6)은 M9 밸런스에서
		Reward.GoldAdded = FMath::RoundToInt(Report.Gold * (1.f + FamilyGoldPercent / 100.f));
		SaveData->Gold += Reward.GoldAdded;
	}

	// design §8.7 길드 누적 기록: 처치는 사망해도 누적, 탈출·골드는 탈출 시만
	FMLGuildStats& Stats = SaveData->GuildStats;
	Stats.Kills += Report.Kills;
	Stats.EliteKills += Report.EliteKills;
	if (Report.bBossKilled) ++Stats.BossKills;
	if (Report.bExtracted)
	{
		++Stats.Extractions;
		Stats.GoldEarned += Reward.GoldAdded;
	}
	else if (SaveData->bHasMerc)
	{
		++Stats.Deaths;
	}

	for (const FName& Id : GetTitleIds())
	{
		const FMLTitleRow* Title = FindTitle(Id);
		if (Title && IsTitleUnlocked(Id) && !UnlockedBefore.Contains(Id)) Reward.NewTitles.Add(Title->Name);
	}

	// design §6.2.3: 되찾았거나 기한이 끝난 묘비 제거 (이번에 죽은 용병의 묘비는 아래에서 추가)
	for (FMLGrave& Grave : SaveData->Graves)
	{
		--Grave.RunsLeft;
	}
	SaveData->Graves.RemoveAll([&Report](const FMLGrave& Grave)
	{
		return Grave.RunsLeft <= 0 || Grave.MercId == Report.ClearedGraveMercId;
	});

	// design §6.3: 원정 아이템은 탈출할 때만 창고에 들어감
	if (Report.bExtracted)
	{
		Reward.AutoSoldGold = AddToStash(Report.Items);
		for (const FName& Id : Report.Items)
		{
			if (const FMLItemRow* Item = FindItem(Id)) Reward.ItemNames.Add(Item->Name);
		}
	}
	else
	{
		Reward.ItemsLost = Report.Items.Num();
	}

	if (SaveData->bHasMerc)
	{
		FMLMercData& Merc = SaveData->Merc;
		Reward.bHasMerc = true;
		Reward.MercName = Merc.Name;
		Reward.OldLevel = Merc.Level;
		Merc.BestZone = FMath::Max(Merc.BestZone, Report.Zone);

		if (Report.bExtracted)
		{
			// design §7.1.1: (처치 + 엘리트 × 10 + 도달 보너스) × 도달 구역 보상 배율
			const UMLGuildSettings* Settings = GetDefault<UMLGuildSettings>();
			const int32 ReachBonus = Report.bBossKilled ? Settings->BossBonusXP
				: (Settings->ZoneBonusXP.IsValidIndex(Report.Zone - 1) ? Settings->ZoneBonusXP[Report.Zone - 1] : 0);
			Reward.MercXP = FMath::RoundToInt((Report.Kills * Settings->KillXP + Report.EliteKills * Settings->EliteKillXP + ReachBonus)
				* Report.RewardMult * (1.f + FamilyXPPercent / 100.f));
			GainMercXP(Merc, Reward.MercXP);

			++Merc.Extractions;
			if (Report.bBossKilled) ++Merc.BossKills;
			Merc.GoldEarned += Reward.GoldAdded;
			Reward.NewLevel = Merc.Level;
		}
		else
		{
			// design §6.6: 사망 = 용병 영구 소실 (장비 소실은 M7-3에서)
			Reward.bFallen = true;
			Reward.NewLevel = Merc.Level;
			Reward.ItemsLost += Merc.Equipped.Num(); // design §8.2: 장착 아이템은 파괴됨 (명예의 전당 기록에는 남음)

			// design §6.2.3: 장착 무기의 방식으로, 없으면 주무기 방식으로 싸우는 망령이 됨
			const FName* WeaponId = Merc.Equipped.Find(EMLItemSlot::Weapon);
			const FMLItemRow* Weapon = WeaponId ? FindItem(*WeaponId) : nullptr;
			if (LeaveGrave(Merc, Report.Zone, Weapon ? Weapon->WeaponType : Merc.WeaponType, Report.Skills, Report.Passives)) Reward.GraveZone = Report.Zone;
			Reward.HallNumber = MoveMercToHall(EMLLeaveReason::Fallen);
		}
	}
	
	// design §8.5.1 길드 의뢰: 탈출 시에만 지급, 가문 보너스 미적용
	if (const FMLRequestRow* Request = FindRequest(SaveData->ActiveRequest))
	{
		Reward.RequestName = Request->Name;
		Reward.RequestProgress = RequestProgress(*Request, Report);
		Reward.RequestTarget = Request->Target;
		Reward.bRequestDone = Report.bExtracted && Reward.RequestProgress >= Request->Target;
		if (Reward.bRequestDone)
		{
			Reward.RequestGold = Request->RewardGold;
			Reward.RequestXP = Request->RewardXP;
			Reward.RequestPoints = Request->RewardPoints;
			SaveData->Gold += Request->RewardGold;
			SaveData->FamilyPoints += Request->RewardPoints;
			if (SaveData->bHasMerc)
			{
				GainMercXP(SaveData->Merc, Request->RewardXP);
				Reward.NewLevel = SaveData->Merc.Level;
			}
		}
	}
	SaveData->ActiveRequest = NAME_None;

	RefreshCandidates(); // design §8.1: 원정마다 새 후보
	RefreshShop();       // design §8.4: 원정마다 새 상점 진열
	RefreshRequests();   // design §8.5.1: 원정마다 새 의뢰판
	Save();
	
	return Reward;
}

bool UMLGuildSubsystem::GetActiveStats(FMLStatBlock& OutStats) const
{
	if (!SaveData || !SaveData->bHasMerc) return false;
	OutStats = ComputeStats(SaveData->Merc);
	return true;
}

float UMLGuildSubsystem::GetEnemyScale() const
{
	const FMLGradeRow* Row = SaveData && SaveData->bHasMerc ? FindGrade(SaveData->Merc.Grade) : nullptr;
	return Row ? Row->EnemyScale : 1.f;
}

FMLStatBlock UMLGuildSubsystem::ComputeStats(const FMLMercData& Merc) const
{
	const FMLGradeRow* Row = FindGrade(Merc.Grade);
	if (!Row) return Merc.BaseStats;

	// 웹 mercenary.js leveledStats
	const float L1 = static_cast<float>(Merc.Level - 1);
	const float LevelMul = 1.f + L1 * (Row->Growth - 1.f) * 0.5f + L1 * 0.06f * Row->Growth;
	const float PercentMul = 1.f + L1 * 0.012f * Row->Growth;

	FMLStatBlock Out;
	for (int32 i = 0; i < FMLStatBlock::Num; ++i)
	{
		Out[i] = Merc.BaseStats[i] * (FMLStatBlock::IsPercentStat(i) ? PercentMul : LevelMul);
	}

	for (const FName& TalentId : Merc.Talents)
	{
		const FMLTalentRow* Talent = TalentTable ? TalentTable->FindRow<FMLTalentRow>(TalentId, TEXT("MLGuild")) : nullptr;
		if (!Talent) continue;
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			Out[i] = Out[i] * (1.f + Talent->BonusPercent[i] / 100.f) + Talent->Flat[i];
		}
	}

	for (int32 i = 0; i < FMLStatBlock::Num; ++i)
	{
		Out[i] += Merc.Training[i]; // design §7.5
	}

	for (const TPair<EMLItemSlot, FName>& Pair : Merc.Equipped)
	{
		const FMLItemRow* Item = FindItem(Pair.Value);
		if (!Item) continue;
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			Out[i] += Item->Flat[i]; // design §7.2 순서: 훈련 다음, 가문 이전
		}
	}

	FMLStatBlock FamilyStatPercent;
	float UnusedGold = 0.f;
	float UnusedXP = 0.f;
	GetFamilyBonuses(FamilyStatPercent, UnusedGold, UnusedXP);
	for (int32 i = 0; i < FMLStatBlock::Num; ++i)
	{
		Out[i] *= 1.f + FamilyStatPercent[i] / 100.f; // design §8.3 가문의 힘/방패/혈통
	}

	// design §8.7: 칭호 (가문 다음, 재능과 같은 비율 + 고정값)
	if (const FMLTitleRow* Title = GetMercTitle(Merc))
	{
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			Out[i] = Out[i] * (1.f + Title->BonusPercent[i] / 100.f) + Title->Flat[i];
		}
	}

	return Out;
}

const FMLTitleRow* UMLGuildSubsystem::FindTitle(FName TitleId) const
{
	return TitleTable && !TitleId.IsNone() ? TitleTable->FindRow<FMLTitleRow>(TitleId, TEXT("MLGuild")) : nullptr;
}

TArray<FName> UMLGuildSubsystem::GetTitleIds() const
{
	return TitleTable ? TitleTable->GetRowNames() : TArray<FName>();
}

int32 UMLGuildSubsystem::GetTitleProgress(FName TitleId) const
{
	const FMLTitleRow* Title = FindTitle(TitleId);
	return Title && SaveData ? SaveData->GuildStats.Get(Title->Stat) : 0;
}

bool UMLGuildSubsystem::IsTitleUnlocked(FName TitleId) const
{
	const FMLTitleRow* Title = FindTitle(TitleId);
	return Title && GetTitleProgress(TitleId) >= Title->Target;
}

const FMLTitleRow* UMLGuildSubsystem::GetMercTitle(const FMLMercData& Merc) const
{
	return IsTitleUnlocked(Merc.Title) ? FindTitle(Merc.Title) : nullptr;
}

bool UMLGuildSubsystem::EquipTitle(FName TitleId)
{
	if (!CanAct(TEXT("equip title")) || !SaveData->bHasMerc) return false;
	if (!TitleId.IsNone() && !IsTitleUnlocked(TitleId)) return false;

	SaveData->Merc.Title = TitleId;

	Save();
	return true;
}

bool UMLGuildSubsystem::GetActiveTitle(FText& OutName, EMLTitleRarity& OutRarity) const
{
	const FMLTitleRow* Title = SaveData && SaveData->bHasMerc ? GetMercTitle(SaveData->Merc) : nullptr;
	if (!Title) return false;

	OutName = Title->Name;
	OutRarity = Title->Rarity;
	return true;
}

int32 UMLGuildSubsystem::GetXPToNextLevel(const FMLMercData& Merc) const
{
	const FMLGradeRow* Row = FindGrade(Merc.Grade);
	if (!Row || Merc.Level >= Row->MaxLevel) return 0;

	// design §7.1.1: 웹 곡선 50 × L^1.55, TargetRuns 평균 원정으로 최대 레벨에 도달하도록 배율 조정
	double WebTotal = 0.0;
	for (int32 L = 1; L < Row->MaxLevel; ++L)
	{
		WebTotal += 50.0 * FMath::Pow(static_cast<double>(L), 1.55);
	}
	const double Scale = Row->TargetRuns * GetDefault<UMLGuildSettings>()->ExpectedRunXP / FMath::Max(1.0, WebTotal);
	return FMath::Max(1, FMath::FloorToInt(static_cast<float>(50.0 * FMath::Pow(static_cast<double>(Merc.Level), 1.55) * Scale)));
}

void UMLGuildSubsystem::GainMercXP(FMLMercData& Merc, int32 Amount) const
{
	const FMLGradeRow* Row = FindGrade(Merc.Grade);
	if (!Row) return;

	Merc.XP += Amount;
	while (Merc.Level < Row->MaxLevel)
	{
		const int32 Need = GetXPToNextLevel(Merc);
		if (Merc.XP < Need) break;
		Merc.XP -= Need;
		++Merc.Level;
	}
	if (Merc.Level >= Row->MaxLevel) Merc.XP = 0;
}

const FMLGradeRow* UMLGuildSubsystem::FindGrade(EMLGrade Grade) const
{
	return GradeTable ? GradeTable->FindRow<FMLGradeRow>(FName(*GradeName(Grade)), TEXT("MLGuild")) : nullptr;
}

FMLMercData UMLGuildSubsystem::MakeMerc(EMLGrade Grade)
{
	FMLMercData Merc;
	Merc.Id = SaveData->NextMercId++;
	// shortcut: 후보 ≤21명에 이름 40개, 무작위 20번 시도; 이름이 겹치면 섞은 풀로 교체
	for (int32 Try = 0; Try < 20; ++Try)
	{
		Merc.Name = MercNames[FMath::RandRange(0, UE_ARRAY_COUNT(MercNames) - 1)];
		const bool bTaken = (SaveData->bHasMerc && SaveData->Merc.Name == Merc.Name)
			|| SaveData->Candidates.ContainsByPredicate([&Merc](const FMLMercData& Other) { return Other.Name == Merc.Name; });
		if (!bTaken) break;
	}
	Merc.Grade = Grade;
	
	// design §7.4 주무기. 단검은 데이터가 없을 때의 대체값일 뿐, 뽑히지 않음.
	const EMLAttackType MainWeapons[] = { EMLAttackType::Sword, EMLAttackType::Bow, EMLAttackType::Staff };
	Merc.WeaponType = MainWeapons[FMath::RandRange(0, UE_ARRAY_COUNT(MainWeapons) - 1)];

	const FMLGradeRow* Row = FindGrade(Grade);
	if (!Row) return Merc;

	for (int32 i = 0; i < FMLStatBlock::Num; ++i)
	{
		Merc.BaseStats[i] = static_cast<float>(FMath::RandRange(FMath::RoundToInt(Row->BaseMin[i]), FMath::RoundToInt(Row->BaseMax[i])));
	}

	if (!TalentTable) return Merc;

	// design §7.3: 가중치 추첨, 중복 없음, 재능 슬롯마다 하나. design §8.3: 가문 재능은 ×FamilyTalentWeightMult 가중
	const float FamilyMult = GetDefault<UMLGuildSettings>()->FamilyTalentWeightMult;
	auto WeightOf = [this, FamilyMult](FName Id)
	{
		const FMLTalentRow* Talent = TalentTable->FindRow<FMLTalentRow>(Id, TEXT("MLGuild"));
		const float Weight = Talent ? FMath::Max(0.f, Talent->Weight) : 0.f;
		return SaveData->FamilyTalents.Contains(Id) ? Weight * FamilyMult : Weight;
	};
	
	TArray<FName> Pool = TalentTable->GetRowNames();
	for (int32 Slot = 0; Slot < Row->TalentSlots && Pool.Num() > 0; ++Slot)
	{
		float Total = 0.f;
		for (const FName& Id : Pool) Total += WeightOf(Id);

		float Roll = FMath::FRandRange(0.f, Total);
		int32 Picked = Pool.Num() - 1;
		for (int32 k = 0; k < Pool.Num(); ++k)
		{
			Roll -= WeightOf(Pool[k]);
			if (Roll <= 0.f)
			{
				Picked = k;
				break;
			}
		}
		Merc.Talents.Add(Pool[Picked]);
		Pool.RemoveAt(Picked);
	}
	return Merc;
}

void UMLGuildSubsystem::RefreshCandidates()
{
	SaveData->Candidates.Reset();
	AddMissingCandidates();
}

int32 UMLGuildSubsystem::AddMissingCandidates()
{
	int32 Added = 0;
	const int32 PerGrade = GetDefault<UMLGuildSettings>()->CandidatesPerGrade;
	for (int32 g = 0; g < static_cast<int32>(EMLGrade::Count); ++g)
	{
		const EMLGrade Grade = static_cast<EMLGrade>(g);
		const FMLGradeRow* Row = FindGrade(Grade);
		if (!Row || SaveData->Renown < Row->UnlockRenown) continue; // design §8.3 명성에 따른 등급 해금
		if (SaveData->Candidates.ContainsByPredicate([Grade](const FMLMercData& C) { return C.Grade == Grade; })) continue;

		for (int32 k = 0; k < PerGrade; ++k)
		{
			SaveData->Candidates.Add(MakeMerc(Grade));
			++Added;
		}
	}
	return Added;
}

FString UMLGuildSubsystem::TalentsToString(const TArray<FName>& Talents) const
{
	TArray<FString> Names;
	for (const FName& Id : Talents)
	{
		const FMLTalentRow* Talent = TalentTable ? TalentTable->FindRow<FMLTalentRow>(Id, TEXT("MLGuild")) : nullptr;
		Names.Add(Talent ? Talent->Name.ToString() : Id.ToString());
	}
	return FString::Join(Names, TEXT(", "));
}

bool UMLGuildSubsystem::CanAct(const TCHAR* Action) const
{
	if (SaveData && !SaveData->bRunInProgress) return true;
	UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: %s refused (run in progress)"), Action);
	return false;
}

int32 UMLGuildSubsystem::MoveMercToHall(EMLLeaveReason Reason)
{
	// design §8.5. shortcut: 장착 아이템은 M7-3에서 창고로 가거나(은퇴/판매) 파괴됨(전사)
	FMLHallEntry& Entry = SaveData->HallOfFame.AddDefaulted_GetRef();
	Entry.Merc = SaveData->Merc;
	Entry.Reason = Reason;
	Entry.Date = FDateTime::Now();

	SaveData->bHasMerc = false;
	SaveData->Merc = FMLMercData();
	return SaveData->HallOfFame.Num();
}

bool UMLGuildSubsystem::LeaveGrave(const FMLMercData& Merc, int32 Zone, EMLAttackType WeaponType, const TArray<FSoftClassPath>& Skills, const TArray<FName>& Passives)
{
	if (Merc.Equipped.Num() == 0) return false; // 되찾을 장비가 없음

	FMLGrave Grave;
	Grave.MercId = Merc.Id;
	Grave.Name = Merc.Name;
	Grave.Grade = Merc.Grade;
	Grave.WeaponType = WeaponType;
	Merc.Equipped.GenerateValueArray(Grave.Items);
	Grave.Zone = Zone;
	Grave.RunsLeft = GetDefault<UMLGuildSettings>()->GraveRuns;
	Grave.Skills = Skills;
	Grave.Passives = Passives;
	SaveData->Graves.Add(Grave);

	const int32 Excess = SaveData->Graves.Num() - GetDefault<UMLGuildSettings>()->MaxGraves;
	if (Excess > 0) SaveData->Graves.RemoveAt(0, Excess); // 가장 오래된 것부터
	return true;
}

const FMLGrave* UMLGuildSubsystem::FindGraveInZone(int32 Zone) const
{
	return SaveData ? SaveData->Graves.FindByPredicate([Zone](const FMLGrave& Grave) { return Grave.Zone == Zone; }) : nullptr;
}

void UMLGuildSubsystem::GetFamilyBonuses(FMLStatBlock& OutStatPercent, float& OutGoldPercent, float& OutXPPercent) const
{
	OutStatPercent = FMLStatBlock();
	OutGoldPercent = 0.f;
	OutXPPercent = 0.f;
	if (!SaveData || !FamilyUpgradeTable) return;

	for (const TPair<FName, int32>& Pair : SaveData->FamilyUpgrades)
	{
		const FMLFamilyUpgradeRow* Row = FamilyUpgradeTable->FindRow<FMLFamilyUpgradeRow>(Pair.Key, TEXT("MLGuild"));
		if (!Row) continue;
		for (int32 i = 0; i < FMLStatBlock::Num; ++i)
		{
			OutStatPercent[i] += Row->StatPercentPerLevel[i] * Pair.Value;
		}
		OutGoldPercent += Row->GoldPercentPerLevel * Pair.Value;
		OutXPPercent += Row->XPPercentPerLevel * Pair.Value;
	}
}

int32 UMLGuildSubsystem::GetRetirePoints(const FMLMercData& Merc) const
{
	const FMLGradeRow* Row = FindGrade(Merc.Grade);
	if (!Row) return 0;

	// design §8.2: 등급 기본 + floor(lv/10) + floor((탈출 + 보스×3 + 최고 구역)/5), 최대 레벨이면 ×1.5
	const int32 Points = Row->RetirePoints + Merc.Level / 10 + (Merc.Extractions + Merc.BossKills * 3 + Merc.BestZone) / 5;
	return Merc.Level >= Row->MaxLevel ? FMath::FloorToInt(Points * GetDefault<UMLGuildSettings>()->MaxLevelRetireMult) : Points;
}

int32 UMLGuildSubsystem::GetSellPrice(const FMLMercData& Merc) const
{
	const FMLGradeRow* Row = FindGrade(Merc.Grade);
	if (!Row) return 0;

	// design §8.2 = 웹 sellPrice: 계약금 × 0.4 + 50 + lv × 8 × (티어 + 1)
	const int32 Tier = static_cast<int32>(Merc.Grade);
	return FMath::RoundToInt(Row->ContractGold * 0.4f + 50.f + Merc.Level * 8.f * (Tier + 1));
}

bool UMLGuildSubsystem::Retire(int32 TalentIndex, int32 ReplaceSlot)
{
	if (!CanAct(TEXT("retire"))) return false;
	if (!SaveData->bHasMerc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: retire refused (no merc)"));
		return false;
	}

	const FMLMercData Merc = SaveData->Merc; // 복사: MoveMercToHall이 비워 버림
	if (!Merc.Talents.IsValidIndex(TalentIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: retire refused (talent index 0..%d)"), Merc.Talents.Num() - 1);
		return false;
	}

	// design §8.3 유산 재능: 이미 등록됨 = 변화 없음; 목록이 가득 참 = 교체할 슬롯을 지정해야 함
	const FName Legacy = Merc.Talents[TalentIndex];
	const int32 MaxFamily = GetDefault<UMLGuildSettings>()->MaxFamilyTalents;
	TArray<FName>& Family = SaveData->FamilyTalents;
	const bool bAlready = Family.Contains(Legacy);
	if (!bAlready && Family.Num() >= MaxFamily && !Family.IsValidIndex(ReplaceSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: retire refused (family talents full: give ReplaceSlot 0..%d)"), Family.Num() - 1);
		return false;
	}
	if (!bAlready)
	{
		if (Family.Num() < MaxFamily) Family.Add(Legacy);
		else Family[ReplaceSlot] = Legacy;
	}

	const int32 Points = GetRetirePoints(Merc);
	SaveData->FamilyPoints += Points;
	SaveData->Renown += Points;
	ReturnEquipmentToStash(); // design §8.2: 은퇴하면 장비가 창고로
	MoveMercToHall(EMLLeaveReason::Retired);
	AddMissingCandidates();   // 새로 해금된 등급이 바로 표시됨
	Save();
	return true;
}

bool UMLGuildSubsystem::Sell()
{
	if (!CanAct(TEXT("sell"))) return false;
	if (!SaveData->bHasMerc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: sell refused (no merc)"));
		return false;
	}

	SaveData->Gold += GetSellPrice(SaveData->Merc);
	ReturnEquipmentToStash(); // design §8.2: 판매하면 장비가 창고로
	MoveMercToHall(EMLLeaveReason::Sold);
	Save();
	return true;
}

bool UMLGuildSubsystem::Train(FName TrainingId)
{
	if (!CanAct(TEXT("train"))) return false;
	const FMLTrainingRow* Row = FindTraining(TrainingId);
	if (!SaveData->bHasMerc || !Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: train %s refused (no merc or unknown training)"), *TrainingId.ToString());
		return false;
	}

	FMLMercData& Merc = SaveData->Merc;
	const int32 Buys = Merc.TrainingBuys.FindRef(TrainingId); // FindRef: 거절된 구매가 0 항목을 추가하면 안 됨
	const int32 Cost = GetTrainingCost(Merc, TrainingId);
	if (SaveData->Gold < Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: train %s refused (gold %d < %d)"), *TrainingId.ToString(), SaveData->Gold, Cost);
		return false;
	}

	SaveData->Gold -= Cost;
	for (int32 i = 0; i < FMLStatBlock::Num; ++i)
	{
		Merc.Training[i] += Row->Amount[i];
	}
	Merc.TrainingBuys.Add(TrainingId, Buys + 1);
	Save();
	return true;
}

bool UMLGuildSubsystem::BuyShopItem(int32 OfferIndex)
{
	if (!CanAct(TEXT("buy item"))) return false;
	const FName ItemId = SaveData->ShopOffers.IsValidIndex(OfferIndex) ? SaveData->ShopOffers[OfferIndex] : NAME_None;
	const FMLItemRow* Item = FindItem(ItemId);
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: buy refused (offer index not in 0..%d)"), SaveData->ShopOffers.Num() - 1);
		return false;
	}
	if (SaveData->Stash.Num() >= GetDefault<UMLGuildSettings>()->StashCapacity)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: buy %s refused (stash full)"), *ItemId.ToString());
		return false;
	}
	if (SaveData->Gold < Item->Price)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: buy %s refused (gold %d < %d)"), *ItemId.ToString(), SaveData->Gold, Item->Price);
		return false;
	}

	SaveData->Gold -= Item->Price;
	SaveData->ShopOffers.RemoveAt(OfferIndex);
	SaveData->Stash.Add(ItemId);
	Save();
	return true;
}

int32 UMLGuildSubsystem::GetShopUpgradeCost() const
{
	const TArray<int32>& Costs = GetDefault<UMLGuildSettings>()->ShopUpgradeCosts;
	const int32 Index = SaveData ? SaveData->ShopGrade - 1 : INDEX_NONE;
	return Costs.IsValidIndex(Index) ? Costs[Index] : 0;
}

bool UMLGuildSubsystem::UpgradeShop()
{
	if (!CanAct(TEXT("upgrade shop"))) return false;
	const int32 Cost = GetShopUpgradeCost();
	if (Cost <= 0 || SaveData->Gold < Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: upgrade shop refused (grade=%d gold %d < %d, cost 0 = max grade)"),
			SaveData->ShopGrade, SaveData->Gold, Cost);
		return false;
	}

	SaveData->Gold -= Cost;
	++SaveData->ShopGrade;
	RefreshShop();
	Save();
	return true;
}

void UMLGuildSubsystem::RefreshShop()
{
	SaveData->ShopOffers.Reset();

	// [MinTier, MaxTier] 아이템 중 아직 진열되지 않은 것을 Count개까지 추가
	auto Draw = [this](int32 MinTier, int32 MaxTier, int32 Count)
	{
		TArray<FName> Pool;
		if (ItemTable)
		{
			ItemTable->ForeachRow<FMLItemRow>(TEXT("MLGuild"), [&](const FName& Key, const FMLItemRow& Item)
			{
				if (Item.Tier >= MinTier && Item.Tier <= MaxTier && !SaveData->ShopOffers.Contains(Key)) Pool.Add(Key);
			});
		}
		for (; Count > 0 && Pool.Num() > 0; --Count)
		{
			const int32 Pick = FMath::RandRange(0, Pool.Num() - 1);
			SaveData->ShopOffers.Add(Pool[Pick]);
			Pool.RemoveAtSwap(Pick); // 서로 다른 진열
		}
	};

	// design §8.4: 등급 N = N~N+1티어. 2등급부터 1칸은 항상 1티어 (전사 후 F 용병도 살 것이 남게)
	const int32 Grade = SaveData->ShopGrade;
	const int32 BasicCount = Grade > 1 ? 1 : 0;
	Draw(Grade, Grade + 1, GetDefault<UMLGuildSettings>()->ShopOfferCount - BasicCount);
	Draw(1, 1, BasicCount);
}

bool UMLGuildSubsystem::AcceptRequest(int32 OfferIndex)
{
	if (!CanAct(TEXT("accept request"))) return false;
	SaveData->ActiveRequest = SaveData->RequestOffers.IsValidIndex(OfferIndex) ? SaveData->RequestOffers[OfferIndex] : NAME_None;
	Save();

	return true;
}

bool UMLGuildSubsystem::ConsumeRollbackNotice()
{
	const bool bNotice = bRollbackNotice;
	bRollbackNotice = false;
	return bNotice;
}

void UMLGuildSubsystem::RefreshRequests()
{
	SaveData->RequestOffers.Reset();
	if (!RequestTable) return;

	TArray<FName> Pool = RequestTable->GetRowNames();
	const int32 Count = GetDefault<UMLGuildSettings>()->RequestOfferCount;
	while (SaveData->RequestOffers.Num() < Count && Pool.Num() > 0)
	{
		float Total = 0.f;
		for (const FName& Id : Pool) Total += FMath::Max(0.f, FindRequest(Id)->Weight);

		float Roll = FMath::FRandRange(0.f, Total);
		int32 Picked = Pool.Num() - 1;
		for (int32 k = 0; k < Pool.Num(); ++k)
		{
			Roll -= FMath::Max(0.f, FindRequest(Pool[k])->Weight);
			if (Roll <= 0.f)
			{
				Picked = k;
				break;
			}
		}
		SaveData->RequestOffers.Add(Pool[Picked]);
		Pool.RemoveAt(Picked); // 서로 다른 진열
	}
}

bool UMLGuildSubsystem::BuyFamilyUpgrade(FName UpgradeId)
{
	if (!CanAct(TEXT("upgrade"))) return false;
	const FMLFamilyUpgradeRow* Row = FindFamilyUpgrade(UpgradeId);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: unknown upgrade %s"), *UpgradeId.ToString());
		return false;
	}

	const int32 Level = SaveData->FamilyUpgrades.FindRef(UpgradeId); // FindRef: 거절된 구매가 0 항목을 추가하면 안 됨
	const int32 Cost = GetFamilyUpgradeCost(UpgradeId);
	if (Level >= Row->MaxLevel || SaveData->FamilyPoints < Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: upgrade %s refused (lv %d/%d, points %d < %d)"),
			*UpgradeId.ToString(), Level, Row->MaxLevel, SaveData->FamilyPoints, Cost);
		return false;
	}

	SaveData->FamilyPoints -= Cost;
	SaveData->FamilyUpgrades.Add(UpgradeId, Level + 1);
	Save();
	return true;
}

bool UMLGuildSubsystem::RerollCandidates()
{
	if (!CanAct(TEXT("reroll"))) return false;

	const int32 Cost = GetRerollCost();
	if (SaveData->Gold < Cost)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: reroll refused (gold %d < %d)"), SaveData->Gold, Cost);
		return false;
	}

	SaveData->Gold -= Cost;
	++SaveData->RerollCount;
	RefreshCandidates();
	Save();

	return true;
}

void UMLGuildSubsystem::AddPoints(int32 Amount)
{
	if (!SaveData) return;
	SaveData->FamilyPoints = FMath::Max(0, SaveData->FamilyPoints + Amount);
	SaveData->Renown = FMath::Max(0, SaveData->Renown + Amount);
	const int32 Added = AddMissingCandidates();
	Save();
	UE_LOG(LogTemp, Display, TEXT("[ML] Guild: points %+d -> family=%d renown=%d candidates+%d"), Amount, SaveData->FamilyPoints, SaveData->Renown, Added);
}

const UMLSaveGame* UMLGuildSubsystem::GetSaveData() const
{
	return SaveData;
}

const FMLTalentRow* UMLGuildSubsystem::FindTalent(FName TalentId) const
{
	return TalentTable && !TalentId.IsNone() ? TalentTable->FindRow<FMLTalentRow>(TalentId, TEXT("MLGuild")) : nullptr;
}

int32 UMLGuildSubsystem::GetRerollCost() const
{
	const UMLGuildSettings* Settings = GetDefault<UMLGuildSettings>();
	return SaveData ? FMath::RoundToInt(Settings->RerollBaseGold * FMath::Pow(Settings->RerollGrowth, static_cast<float>(SaveData->RerollCount))) : 0;
}

int32 UMLGuildSubsystem::GetItemSellPrice(const FMLItemRow& Item) const
{
	return FMath::RoundToInt(Item.Price * GetDefault<UMLGuildSettings>()->ItemSellRate); // design §8.4
}

const FMLTrainingRow* UMLGuildSubsystem::FindTraining(FName TrainingId) const
{
	return TrainingTable ? TrainingTable->FindRow<FMLTrainingRow>(TrainingId, TEXT("MLGuild")) : nullptr;
}

TArray<FName> UMLGuildSubsystem::GetTrainingIds() const
{
	return TrainingTable ? TrainingTable->GetRowNames() : TArray<FName>();
}

int32 UMLGuildSubsystem::GetTrainingCost(const FMLMercData& Merc, FName TrainingId) const
{
	const FMLTrainingRow* Row = FindTraining(TrainingId);
	return Row ? FMath::RoundToInt(Row->Cost * FMath::Pow(Row->CostGrowth, static_cast<float>(Merc.TrainingBuys.FindRef(TrainingId)))) : 0;
}

const FMLFamilyUpgradeRow* UMLGuildSubsystem::FindFamilyUpgrade(FName UpgradeId) const
{
	return FamilyUpgradeTable ? FamilyUpgradeTable->FindRow<FMLFamilyUpgradeRow>(UpgradeId, TEXT("MLGuild")) : nullptr;
}

TArray<FName> UMLGuildSubsystem::GetFamilyUpgradeIds() const
{
	return FamilyUpgradeTable ? FamilyUpgradeTable->GetRowNames() : TArray<FName>();
}

int32 UMLGuildSubsystem::GetFamilyUpgradeCost(FName UpgradeId) const
{
	const FMLFamilyUpgradeRow* Row = FindFamilyUpgrade(UpgradeId);
	const int32 Level = SaveData ? SaveData->FamilyUpgrades.FindRef(UpgradeId) : 0;
	return Row ? FMath::RoundToInt(Row->Cost * FMath::Pow(Row->CostGrowth, static_cast<float>(Level))) : 0;
}

const FMLRequestRow* UMLGuildSubsystem::FindRequest(FName RequestId) const
{
	// IsNone 확인: 의뢰를 받지 않은 것이 정상 경우이므로 FindRow가 경고하지 않게 함
	return RequestTable && !RequestId.IsNone() ? RequestTable->FindRow<FMLRequestRow>(RequestId, TEXT("MLGuild")) : nullptr;
}

const FMLItemRow* UMLGuildSubsystem::FindItem(FName ItemId) const
{
	return ItemTable && !ItemId.IsNone() ? ItemTable->FindRow<FMLItemRow>(ItemId, TEXT("MLGuild")) : nullptr;
}

FName UMLGuildSubsystem::RollItem(int32 Step, int32& OutTier) const
{
	OutTier = 0;
	// shortcut: 용병이 없으면 F 테이블로 추첨 (원정 중엔 불가능: BeginRun이 F를 자동 계약)
	const FMLGradeRow* Row = FindGrade(SaveData && SaveData->bHasMerc ? SaveData->Merc.Grade : EMLGrade::F);
	if (!Row || !ItemTable) return NAME_None;

	// design §7.4: 단계 1..3 = 구역, 4 = 보스 테이블
	const FVector4& Weights = Step <= 1 ? Row->DropZone1 : Step == 2 ? Row->DropZone2 : Step == 3 ? Row->DropZone3 : Row->DropBoss;
	double Roll = FMath::FRandRange(0.0, Weights.X + Weights.Y + Weights.Z + Weights.W);
	int32 Tier = 4;
	for (int32 t = 0; t < 4; ++t)
	{
		Roll -= Weights[t];
		if (Roll <= 0.0)
		{
			Tier = t + 1;
			break;
		}
	}

	// 해당 티어 아이템 중 균등 추첨; 빈 티어는 한 단계 아래로 내려감
	for (; Tier >= 1; --Tier)
	{
		TArray<FName> Pool;
		ItemTable->ForeachRow<FMLItemRow>(TEXT("MLGuild"), [Tier, &Pool](const FName& Key, const FMLItemRow& Item)
		{
			if (Item.Tier == Tier) Pool.Add(Key);
		});
		if (Pool.Num() > 0)
		{
			OutTier = Tier;
			return Pool[FMath::RandRange(0, Pool.Num() - 1)];
		}
	}
	return NAME_None;
}

FName UMLGuildSubsystem::GetActiveWeaponRow(float& OutDamageScale) const
{
	OutDamageScale = 1.f;
	if (!SaveData || !SaveData->bHasMerc) return NAME_None;

	// design §7.4: 장착 무기가 공격 방식을 결정, 없으면 용병 주무기; 같은 종류 = 일치 보너스
	const FMLMercData& Merc = SaveData->Merc;
	EMLAttackType Type = Merc.WeaponType;
	const FName* WeaponId = Merc.Equipped.Find(EMLItemSlot::Weapon);
	if (const FMLItemRow* Weapon = WeaponId ? FindItem(*WeaponId) : nullptr)
	{
		if (Weapon->WeaponType == Merc.WeaponType) OutDamageScale = GetDefault<UMLGuildSettings>()->WeaponMatchBonus;
		Type = Weapon->WeaponType;
	}
	return FName(*WeaponName(Type));
}

int32 UMLGuildSubsystem::AddToStash(const TArray<FName>& ItemIds)
{
	// design §8.4: 창고는 한도가 있음; 넘치면 바로 판매해서 아무것도 조용히 사라지지 않게 함
	const int32 Capacity = GetDefault<UMLGuildSettings>()->StashCapacity;
	int32 SoldGold = 0;
	for (const FName& Id : ItemIds)
	{
		const FMLItemRow* Item = FindItem(Id);
		if (!Item) continue;
		if (SaveData->Stash.Num() < Capacity) SaveData->Stash.Add(Id);
		else SoldGold += GetItemSellPrice(*Item);
	}
	SaveData->Gold += SoldGold;
	return SoldGold;
}

int32 UMLGuildSubsystem::ReturnEquipmentToStash()
{
	TArray<FName> ItemIds;
	SaveData->Merc.Equipped.GenerateValueArray(ItemIds);
	SaveData->Merc.Equipped.Empty();
	return AddToStash(ItemIds);
}

bool UMLGuildSubsystem::Equip(int32 StashIndex)
{
	if (!CanAct(TEXT("equip"))) return false;
	const FName ItemId = SaveData->Stash.IsValidIndex(StashIndex) ? SaveData->Stash[StashIndex] : NAME_None;
	const FMLItemRow* Item = FindItem(ItemId);
	if (!SaveData->bHasMerc || !Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: equip refused (no merc or stash index not in 0..%d)"), SaveData->Stash.Num() - 1);
		return false;
	}

	FMLMercData& Merc = SaveData->Merc;
	if (Merc.Grade < Item->RequiredGrade) // design §7.4 티어 제한
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: equip %s refused (needs grade %s, merc is %s)"),
			*ItemId.ToString(), *GradeName(Item->RequiredGrade), *GradeName(Merc.Grade));
		return false;
	}

	SaveData->Stash.RemoveAt(StashIndex);
	FName& SlotItem = Merc.Equipped.FindOrAdd(Item->Slot);
	if (!SlotItem.IsNone()) SaveData->Stash.Add(SlotItem); // 교체: 창고에 방금 한 칸이 비었으므로 넘침 없음
	SlotItem = ItemId;
	Save();

	return true;
}

bool UMLGuildSubsystem::Unequip(EMLItemSlot Slot)
{
	if (!CanAct(TEXT("unequip"))) return false;
	FName ItemId;
	if (!SaveData->bHasMerc || !SaveData->Merc.Equipped.RemoveAndCopyValue(Slot, ItemId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: unequip %s refused (no merc or empty slot)"), *SlotName(Slot));
		return false;
	}

	AddToStash({ ItemId }); // 창고가 가득 참 = 즉시 판매
	Save();

	return true;
}

bool UMLGuildSubsystem::SellItem(int32 StashIndex)
{
	if (!CanAct(TEXT("sell item"))) return false;
	const FName ItemId = SaveData->Stash.IsValidIndex(StashIndex) ? SaveData->Stash[StashIndex] : NAME_None;
	const FMLItemRow* Item = FindItem(ItemId);
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: sell item refused (stash index not in 0..%d)"), SaveData->Stash.Num() - 1);
		return false;
	}

	const int32 Price = GetItemSellPrice(*Item);
	SaveData->Gold += Price;
	SaveData->Stash.RemoveAt(StashIndex);
	Save();

	return true;
}

void UMLGuildSubsystem::AddItem(FName ItemId)
{
	if (!SaveData || !FindItem(ItemId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: unknown item %s"), *ItemId.ToString());
		return;
	}
	const int32 SoldGold = AddToStash({ ItemId });
	Save();
	UE_LOG(LogTemp, Display, TEXT("[ML] Guild: item %s added (stash=%d autoSold=%d)"), *ItemId.ToString(), SaveData->Stash.Num(), SoldGold);
}

void UMLGuildSubsystem::AddGrave(int32 Zone, EMLAttackType WeaponType, const TArray<FSoftClassPath>& Skills)
{
	if (!SaveData) return;

	FMLMercData Copy = SaveData->Merc;
	Copy.Id = SaveData->NextMercId++; // 시험용 묘비끼리 Id가 겹치지 않게
	if (!SaveData->bHasMerc || !LeaveGrave(Copy, Zone, WeaponType, Skills, TArray<FName>()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] Guild: add grave refused (no merc or nothing equipped)"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, TEXT("AddGrave 거절: 용병이 없거나 장비를 끼지 않음"));
		return;
	}
	Save();
}

void UMLGuildSubsystem::CreateNewSave()
{
	SaveData = Cast<UMLSaveGame>(UGameplayStatics::CreateSaveGameObject(UMLSaveGame::StaticClass()));
	SaveData->bGuildStatsSeeded = true; // 새 저장은 이어받을 기록이 없음
	RefreshCandidates();
	RefreshShop();
	RefreshRequests();
	Save();
}

void UMLGuildSubsystem::Save()
{
	if (!SaveData || !UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, SaveUserIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[ML] Guild: save failed"));
	}
	OnGuildChanged.Broadcast(); // 모든 길드 동작이 여기서 끝나므로 UI가 한 곳에서 갱신됨
}

void UMLGuildSubsystem::LogStatus() const
{
	if (!SaveData) return;

	UE_LOG(LogTemp, Display, TEXT("[ML] Guild status: gold=%d renown=%d inRun=%d"), SaveData->Gold, SaveData->Renown, SaveData->bRunInProgress ? 1 : 0);
	FString Upgrades;
	for (const TPair<FName, int32>& Pair : SaveData->FamilyUpgrades)
	{
		Upgrades += FString::Printf(TEXT("%s=%d "), *Pair.Key.ToString(), Pair.Value);
	}
	const UMLGuildSettings* Settings = GetDefault<UMLGuildSettings>();
	UE_LOG(LogTemp, Display, TEXT("[ML]   family points=%d talents=[%s] upgrades=[%s] rerollNext=%d"),
		SaveData->FamilyPoints, *TalentsToString(SaveData->FamilyTalents), *Upgrades.TrimEnd(), GetRerollCost());
	
	FString StashText;
	for (int32 i = 0; i < SaveData->Stash.Num(); ++i)
	{
		StashText += FString::Printf(TEXT("%d:%s "), i, *SaveData->Stash[i].ToString());
	}
	UE_LOG(LogTemp, Display, TEXT("[ML]   stash %d/%d [%s]"), SaveData->Stash.Num(), Settings->StashCapacity, *StashText.TrimEnd());
	
	if (SaveData->bHasMerc)
	{
		const FMLMercData& Merc = SaveData->Merc;
		FString EquipText;
		for (const TPair<EMLItemSlot, FName>& Pair : Merc.Equipped)
		{
			EquipText += FString::Printf(TEXT("%s=%s "), *SlotName(Pair.Key), *Pair.Value.ToString());
		}
		UE_LOG(LogTemp, Display, TEXT("[ML]   merc %s grade=%s lv=%d xp=%d/%d weapon=%s equip=[%s] runs=%d boss=%d bestZone=%d talents=[%s] training(%s) retire=%d sell=%d final(%s)"),
			*Merc.Name, *GradeName(Merc.Grade), Merc.Level, Merc.XP, GetXPToNextLevel(Merc), *WeaponName(Merc.WeaponType), *EquipText.TrimEnd(),
			Merc.Extractions, Merc.BossKills, Merc.BestZone, *TalentsToString(Merc.Talents), *StatsToString(Merc.Training),
			GetRetirePoints(Merc), GetSellPrice(Merc), *StatsToString(ComputeStats(Merc)));
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[ML]   merc none"));
	}

	TMap<EMLGrade, int32> GradeIndex;
	for (const FMLMercData& Candidate : SaveData->Candidates)
	{
		const FMLGradeRow* Row = FindGrade(Candidate.Grade);
		int32& Index = GradeIndex.FindOrAdd(Candidate.Grade);
		UE_LOG(LogTemp, Display, TEXT("[ML]   candidate %s#%d %s cost=%d weapon=%s talents=[%s] final(%s)"),
			*GradeName(Candidate.Grade), Index++, *Candidate.Name, Row ? Row->ContractGold : 0, *WeaponName(Candidate.WeaponType),
			*TalentsToString(Candidate.Talents), *StatsToString(ComputeStats(Candidate)));
	}

	for (int32 i = 0; i < SaveData->HallOfFame.Num(); ++i)
	{
		const FMLHallEntry& Entry = SaveData->HallOfFame[i];
		UE_LOG(LogTemp, Display, TEXT("[ML]   hall #%d %s grade=%s lv=%d %s runs=%d %s"),
			i + 1, *Entry.Merc.Name, *GradeName(Entry.Merc.Grade), Entry.Merc.Level,
			*StaticEnum<EMLLeaveReason>()->GetNameStringByValue(static_cast<int64>(Entry.Reason)),
			Entry.Merc.Extractions, *Entry.Date.ToString());
	}
}

void UMLGuildSubsystem::ResetSave()
{
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	CreateNewSave();
	UE_LOG(LogTemp, Display, TEXT("[ML] Guild: save reset (candidates=%d)"), SaveData->Candidates.Num());
}

void UMLGuildSubsystem::AddGold(int32 Amount)
{
	if (!SaveData) return;
	SaveData->Gold = FMath::Max(0, SaveData->Gold + Amount);
	Save();
	UE_LOG(LogTemp, Display, TEXT("[ML] Guild: gold %+d -> %d"), Amount, SaveData->Gold);
}
