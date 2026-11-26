// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLRunGameMode.h"

#include "MLPickupActor.h"
#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Enemy/MLEnemyActor.h"
#include "Enemy/MLBossActor.h"
#include "Enemy/MLGhostActor.h"
#include "Enemy/MLEnemySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/IConsoleManager.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/MLPickupSubsystem.h"
#include "Run/MLRoomGateActor.h"
#include "Run/MLRunProgressComponent.h"
#include "Skills/MLAutoSkill.h"
#include "TimerManager.h"
#include "UI/MLRunResultWidget.h"
#include "Engine/GameInstance.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLGuildSettings.h"
#include "Guild/MLSaveGame.h"

namespace
{
	EMLEliteTrait RollEliteTrait()
	{
		return static_cast<EMLEliteTrait>(FMath::RandRange(1, static_cast<int32>(EMLEliteTrait::Count) - 1));
	}

	// 시험용: 정상 전사 흐름(결과 화면, 묘비, 명예의 전당)을 바로 시작
	FAutoConsoleCommandWithWorld RunDieCmd(TEXT("ML.Run.Die"), TEXT("Kill the hero through the normal death flow"),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			if (Hero && World->GetAuthGameMode<AMLRunGameMode>()) Hero->HandleDeath();
		}));
}

AMLRunGameMode::AMLRunGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	RoomsPerZone = { 3, 3, 2 }; // design §6.2 (3구역 다음은 보스 방, M6-4)

	ZoneSpawns.SetNum(3);
	ZoneSpawns[0].SpawnInterval = 0.5f;  ZoneSpawns[0].SpawnPerTick = 2; ZoneSpawns[0].MaxAlive = 120; ZoneSpawns[0].HealthMult = 1.0f;
	ZoneSpawns[1].SpawnInterval = 0.4f;  ZoneSpawns[1].SpawnPerTick = 3; ZoneSpawns[1].MaxAlive = 160; ZoneSpawns[1].HealthMult = 1.8f;
	ZoneSpawns[2].SpawnInterval = 0.3f;  ZoneSpawns[2].SpawnPerTick = 3; ZoneSpawns[2].MaxAlive = 200; ZoneSpawns[2].HealthMult = 3.0f;
	
	ZoneSpawns[0].EnemiesPerRoom = 80;
	ZoneSpawns[1].EnemiesPerRoom = 150;
	ZoneSpawns[2].EnemiesPerRoom = 220;
	
	ZoneSpawns[0].GoldMult = 1.0f;
	ZoneSpawns[1].GoldMult = 1.6f;
	ZoneSpawns[2].GoldMult = 2.5f;

	ZoneSpawns[0].DamageMult = 1.0f;
	ZoneSpawns[1].DamageMult = 1.5f;
	ZoneSpawns[2].DamageMult = 2.2f;
	
	ZoneSpawns[0].ElitesPerRoom = 0; ZoneSpawns[0].EliteRoomElites = 3; // design §6.2 / §6.2.1
	ZoneSpawns[1].ElitesPerRoom = 1; ZoneSpawns[1].EliteRoomElites = 4;
	ZoneSpawns[2].ElitesPerRoom = 2; ZoneSpawns[2].EliteRoomElites = 6;

	GreedThresholds = { 100, 250, 500, 900 }; // design §6.2.2 (M7부터 용병 등급 골드 보정 ×)

	// design §6.2 포털 가중치
	RoomWeights.Add(EMLRoomType::Normal, 45.f);
	RoomWeights.Add(EMLRoomType::Elite, 20.f);
	RoomWeights.Add(EMLRoomType::Gold, 10.f);
	RoomWeights.Add(EMLRoomType::Rest, 12.f);
	RoomWeights.Add(EMLRoomType::Shop, 13.f);
}

void AMLRunGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// design §11: 길드 → 원정 한 번, 영웅이 스폰되어 PossessedBy에서 스탯을 읽기 전에
	if (UMLGuildSubsystem* Guild = GetGuild())
	{
		Guild->BeginRun();
		EnemyHealthScale = Guild->GetEnemyScale();
	}
}

void AMLRunGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	// design §11: 원정은 계약한 용병과 함께 길드 허브에서만 시작 (원정 맵 직접 PIE 포함)
	const UMLGuildSubsystem* Guild = GetGuild();
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (Save && !Save->bHasMerc)
	{
		const TSoftObjectPtr<UWorld>& GuildMap = GetDefault<UMLGuildSettings>()->GuildMap;
		UE_LOG(LogTemp, Warning, TEXT("[ML] Run: blocked (no contracted merc, inRun=%d) -> %s"), Save->bRunInProgress ? 1 : 0, *GuildMap.ToString());
		if (!GuildMap.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(this, GuildMap);
		}
		return; // 방 없음, 스폰 없음, FinishRun 없음
	}

	if (const AActor* Start = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass()))
	{
		ArenaCenter = Start->GetActorLocation();
	}
	if (UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>())
	{
		EnemySub->OnEnemyKilled.AddUObject(this, &AMLRunGameMode::HandleEnemyKilled);
	}
	if (UMLPickupSubsystem* PickupSub = GetWorld()->GetSubsystem<UMLPickupSubsystem>())
	{
		PickupSub->OnVacuumDone.AddUObject(this, &AMLRunGameMode::HandleGemsCollected);
	}

	RunStartSeconds = GetWorld()->GetTimeSeconds();
	// 다음 틱: 그때쯤 영웅과 HUD 설정이 모두 끝남.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AMLRunGameMode::StartRoom, EMLRoomType::Normal));
}

void AMLRunGameMode::HandleHeroDied()
{
	FinishRun(false);
}

int32 AMLRunGameMode::GetRoomsInZone() const
{
	return RoomsPerZone.IsValidIndex(CurrentZone - 1) ? RoomsPerZone[CurrentZone - 1] : 1;
}

void AMLRunGameMode::StartRoom(EMLRoomType NewRoomType)
{
	if (RunState == EMLRunState::Finished) return;

	CurrentRoomType = NewRoomType;
	RoomKills = 0;
	RoomSpawned = 0;
	RoomSecondsLeft = 0;
	RoomEliteCount = 0;
	RoomElitesSpawned = 0;
	RoomElitesKilled = 0;
	bRoomHasPursuer = false;
	bPursuerPending = false;
	bGhostPending = false;
	bGhostWarned = false;
	bGhostDefeated = false;
	// design §6 HUD 피해 리포트: 전투 방만; 방 사이에 준 피해는 버림
	LastRoomDamage.Reset();
	bRoomDamageOpen = NewRoomType != EMLRoomType::Rest && NewRoomType != EMLRoomType::Shop;
	RoomCombatStartSeconds = GetWorld()->GetTimeSeconds();
	if (UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>())
	{
		EnemySub->TakeRoomDamage();
		const float ZoneDamage = GetZoneSpawn().DamageMult;
		EnemySub->SetHeroDamageMult(ZoneDamage * EnemyHealthScale); // design §6.5: 구역 × 용병 등급 보정, 보스 방은 마지막 구역 값
	}

	if (NewRoomType == EMLRoomType::Boss)
	{
		StartBossRoom();
		return;
	}

	if (NewRoomType == EMLRoomType::Rest || NewRoomType == EMLRoomType::Shop)
	{
		StartSafeRoom();
		return;
	}

	// design §6.2.2: 탐욕은 방마다 한 번 읽으므로 방 도중에 어려워지지 않음
	const bool bGoldRoom = NewRoomType == EMLRoomType::Gold;
	const bool bEliteRoom = NewRoomType == EMLRoomType::Elite;
	const FMLZoneSpawnConfig Config = GetZoneSpawn();
	const int32 Greed = GetGreedLevel();
	RoomSpeedMult = 1.f + GreedSpeedPerLevel * Greed;
	// 황금 / 정예 방은 자기 목표로 끝나므로 일반 적이 바닥나지 않음.
	RoomQuota = (bGoldRoom || bEliteRoom) ? MAX_int32 : FMath::RoundToInt(Config.EnemiesPerRoom * (1.f + GreedSpawnPerLevel * Greed));
	RoomEliteCount = bGoldRoom ? 0 : (bEliteRoom ? Config.EliteRoomElites : Config.ElitesPerRoom);

	// design §6.2.2: 탐욕 3 이상이면 모든 전투 방에 추격자 엘리트 추가
	if (Greed >= PursuerGreedLevel)
	{
		bRoomHasPursuer = true;
		bPursuerPending = true;
		GetWorldTimerManager().SetTimer(PursuerTimerHandle, this, &AMLRunGameMode::SpawnPursuer, PursuerDelay, false);
	}

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AMLRunGameMode::SpawnTick, Config.SpawnInterval, true);
	if (bGoldRoom)
	{
		StartGoldRoom();
	}
	else if (bEliteRoom)
	{
		StartEliteRoom();
	}
	else
	{
		RollGhost();
	}

	SetRunState(EMLRunState::Combat);
}

void AMLRunGameMode::RollGhost()
{
	// design §6.2.3: 한 원정에 1번, 원정의 첫 방(기술이 없을 때)은 제외
	const bool bFirstRoom = CurrentZone == 1 && CurrentRoom == 1;
	const UMLGuildSubsystem* Guild = GetGuild();
	const FMLGrave* Grave = Guild ? Guild->FindGraveInZone(CurrentZone) : nullptr;
	if (bGhostOffered || bFirstRoom || !GhostClass || !Grave || FMath::FRand() >= GraveChance) return;

	bGhostOffered = true;
	bGhostPending = true;
	RoomGrave = *Grave;
}

void AMLRunGameMode::SpawnGhost()
{
	const AMLHeroCharacter* Hero = GetHero();
	const UMLGuildSubsystem* Guild = GetGuild();
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	if (RunState != EMLRunState::Combat || !Hero || Hero->IsDead() || !EnemySub || !Save) return;

	// design §6.2.3: 체력 = 구역 배율 × 옛 용병 등급 보정. 옛 등급은 현재 용병 등급 +1까지만 (강제 전투라서)
	const EMLGrade GhostGrade = FMath::Min(RoomGrave.Grade, static_cast<EMLGrade>(static_cast<int32>(Save->Merc.Grade) + 1));
	const FMLGradeRow* GradeRow = Guild->FindGrade(GhostGrade);
	const float GradeScale = GradeRow ? GradeRow->EnemyScale : 1.f;
	const float ZoneHealth = ZoneSpawns.IsValidIndex(CurrentZone - 1) ? ZoneSpawns[CurrentZone - 1].HealthMult : 1.f;
	const FVector ScreenUp = FRotator(0.f, GateFacingYaw, 0.f).Vector();
	int32 GhostIndex = INDEX_NONE;
	if (!EnemySub->SpawnEnemy(GhostClass, Hero->GetActorLocation() + ScreenUp * GhostSpawnDistance, ZoneHealth * GradeScale, 1.f,
		GetZoneSpawn().GoldMult, EMLEliteTrait::None, &GhostIndex))
	{
		GetWorldTimerManager().SetTimer(GhostTimerHandle, this, &AMLRunGameMode::SpawnGhost, 1.f, false); // 풀이 가득 참: 재시도
		return;
	}

	bGhostPending = false;
	GhostActor = Cast<AMLGhostActor>(EnemySub->GetEnemyActor(GhostIndex));
	if (GhostActor) GhostActor->StartGhost(GhostIndex, RoomGrave.WeaponType, RoomGrave.Skills, RoomGrave.Passives);
	GhostSecondsLeft = FMath::CeilToInt(GhostSeconds);
	GetWorldTimerManager().SetTimer(GhostTimerHandle, this, &AMLRunGameMode::GhostSecondTick, 1.f, true);
	OnRoomStateChanged.Broadcast(); // HUD: 안내 → 보스 패널

	// MLVerify(M9-3b): 다음 마일스톤에서 제거
	UE_LOG(LogTemp, Display, TEXT("[MLVerify] Ghost start: id=%d %s zone=%d weapon=%d hpMult=%.2f (zone %.2f x grade %.2f) grade old=%d merc=%d used=%d"),
		RoomGrave.MercId, *RoomGrave.Name, CurrentZone, static_cast<int32>(RoomGrave.WeaponType), ZoneHealth * GradeScale, ZoneHealth, GradeScale,
		static_cast<int32>(RoomGrave.Grade), static_cast<int32>(Save->Merc.Grade), static_cast<int32>(GhostGrade));
}

void AMLRunGameMode::GhostSecondTick()
{
	--GhostSecondsLeft;
	if (GhostSecondsLeft <= 0)
	{
		// design §6.2.3: 물러남, 묘비는 남음
		UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
		if (EnemySub && GhostActor) EnemySub->RemoveEnemy(GhostActor->GetEnemyIndex());
		EndGhost();
		if (IsRoomCleared()) EndRoom(); // 일반 적을 이미 다 잡았으면 더 부를 처치가 없음
	}
	OnRoomStateChanged.Broadcast(); // HUD 카운트다운
}

void AMLRunGameMode::EndGhost()
{
	GetWorldTimerManager().ClearTimer(GhostTimerHandle);
	if (!GhostActor) return;

	GhostActor->StopGhost();
	GhostActor = nullptr;
}

void AMLRunGameMode::StartGoldRoom()
{
	RoomSecondsLeft = FMath::CeilToInt(GoldRoomSeconds);
	GetWorldTimerManager().SetTimer(RoomTimerHandle, this, &AMLRunGameMode::RoomSecondTick, 1.f, true);

	if (!GoldGoblinClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] RunGameMode: GoldGoblinClass not set, gold room has no goblin"));
		return;
	}

	const AMLHeroCharacter* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	if (!Hero || !EnemySub) return;

	const FMLZoneSpawnConfig Config = GetZoneSpawn();
	// 쿼터뷰는 영웅 아래보다 위쪽 땅을 훨씬 많이 보여줌: 보이도록 화면 위쪽에 스폰.
	const FVector ScreenUp = FRotator(0.f, GateFacingYaw, 0.f).Vector();
	const FVector SpawnLoc = Hero->GetActorLocation() + ScreenUp * GoblinSpawnDistance;
	EnemySub->SpawnEnemy(GoldGoblinClass, SpawnLoc, Config.HealthMult, RoomSpeedMult, Config.GoldMult);
}

void AMLRunGameMode::RoomSecondTick()
{
	--RoomSecondsLeft;
	OnRoomStateChanged.Broadcast(); // HUD 카운트다운
	if (RoomSecondsLeft > 0) return;

	EndRoom(); // 슬라임 도망 / 정예 방 시간 초과: 남은 적은 드롭 없이 사라짐
}

void AMLRunGameMode::StartEliteRoom()
{
	RoomSecondsLeft = FMath::CeilToInt(EliteRoomSeconds);
	GetWorldTimerManager().SetTimer(RoomTimerHandle, this, &AMLRunGameMode::RoomSecondTick, 1.f, true);
	// 첫 엘리트는 1초 뒤, 그 후 EliteRoomSpawnInterval마다 한 마리.
	GetWorldTimerManager().SetTimer(EliteTimerHandle, this, &AMLRunGameMode::EliteRoomSpawnTick, EliteRoomSpawnInterval, true, 1.f);
}

void AMLRunGameMode::EliteRoomSpawnTick()
{
	const AMLHeroCharacter* Hero = GetHero();
	if (!Hero || Hero->IsDead()) return;

	if (SpawnElite(GetRingLocation(Hero->GetActorLocation()), RollEliteTrait()))
	{
		++RoomElitesSpawned;
	}
	if (RoomElitesSpawned >= RoomEliteCount)
	{
		GetWorldTimerManager().ClearTimer(EliteTimerHandle);
	}
}

void AMLRunGameMode::SpawnPursuer()
{
	const AMLHeroCharacter* Hero = GetHero();
	if (RunState != EMLRunState::Combat || !Hero || Hero->IsDead()) return;

	// 화면 위쪽 가장자리: 카메라 뒤에서 갑자기 나타나지 않고 다가오는 모습이 보임.
	const FVector ScreenUp = FRotator(0.f, GateFacingYaw, 0.f).Vector();
	if (SpawnElite(Hero->GetActorLocation() + ScreenUp * SpawnRingMin, EMLEliteTrait::Swift))
	{
		bPursuerPending = false;
	}
	else
	{
		GetWorldTimerManager().SetTimer(PursuerTimerHandle, this, &AMLRunGameMode::SpawnPursuer, 1.f, false); // 풀이 가득 참: 재시도
	}
}

bool AMLRunGameMode::SpawnElite(const FVector& Location, EMLEliteTrait Trait)
{
	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	const FMLZoneSpawnConfig Config = GetZoneSpawn();
	if (!EnemySub || !Config.EnemyClass) return false;

	const float GoldMult = Config.GoldMult * (CurrentRoomType == EMLRoomType::Elite ? EliteRoomGoldMult : 1.f);
	return EnemySub->SpawnEnemy(Config.EnemyClass, Location, Config.HealthMult, RoomSpeedMult, GoldMult, Trait);
}

FVector AMLRunGameMode::GetRingLocation(const FVector& HeroLoc) const
{
	const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
	return HeroLoc + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * FMath::FRandRange(SpawnRingMin, SpawnRingMax);
}

void AMLRunGameMode::StartSafeRoom()
{
	RoomQuota = 0;
	SetRunState(EMLRunState::Collecting); // 전투 없음: 휴식터 선택 / 상점을 나간 뒤 포털이 열림

	// 페이드 인을 기다림, 아니면 검은 화면 위에 UI가 열림 (그리고 일시정지).
	GetWorldTimerManager().SetTimer(RoomTimerHandle, this, &AMLRunGameMode::ShowSafeRoomUI, FadeSeconds, false);
}

void AMLRunGameMode::ShowSafeRoomUI()
{
	AMLHeroCharacter* Hero = GetHero();
	UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr;
	if (!RunProgress)
	{
		OpenGates();
		return;
	}

	RunProgress->OnRestFinished.RemoveAll(this);
	RunProgress->OnShopFinished.RemoveAll(this);
	if (CurrentRoomType == EMLRoomType::Shop)
	{
		RunProgress->OnShopFinished.AddUObject(this, &AMLRunGameMode::HandleSafeRoomFinished);
		RunProgress->OpenShop(GetZoneSpawn().GoldMult); // 가격은 금화 가치처럼 구역에 따라 늘어남
	}
	else
	{
		RunProgress->OnRestFinished.AddUObject(this, &AMLRunGameMode::HandleSafeRoomFinished);
		RunProgress->OpenRestChoices();
	}
}

void AMLRunGameMode::HandleSafeRoomFinished()
{
	if (RunState == EMLRunState::Collecting) OpenGates();
}

void AMLRunGameMode::SpawnTick()
{
	const AMLHeroCharacter* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	const FMLZoneSpawnConfig Config = GetZoneSpawn();
	if (!Hero || Hero->IsDead() || !EnemySub || !Config.EnemyClass) return;

	int32 MaxAlive = Config.MaxAlive;
	if (CurrentRoomType == EMLRoomType::Gold) MaxAlive = FMath::Min(MaxAlive, GoldRoomMaxAlive);
	else if (CurrentRoomType == EMLRoomType::Elite) MaxAlive = FMath::Min(MaxAlive, EliteRoomMaxAlive);
	else if (bGhostPending) --MaxAlive; // 3구역은 MaxAlive = 풀 크기: 강적 자리를 비워 둠

	const FVector HeroLoc = Hero->GetActorLocation();
	for (int32 k = 0; k < Config.SpawnPerTick && RoomSpawned < RoomQuota && EnemySub->GetActiveCount() < MaxAlive; ++k)
	{
		if (!EnemySub->SpawnEnemy(Config.EnemyClass, GetRingLocation(HeroLoc), Config.HealthMult, RoomSpeedMult, Config.GoldMult)) break;
		++RoomSpawned;
	}

	// design §6.2: 일반 방은 처치 할당량의 EliteKillStep × k에서 엘리트를 섞음
	if (CurrentRoomType == EMLRoomType::Normal && RoomElitesSpawned < RoomEliteCount
		&& RoomKills >= FMath::CeilToInt(RoomQuota * EliteKillStep * (RoomElitesSpawned + 1)))
	{
		if (SpawnElite(GetRingLocation(HeroLoc), RollEliteTrait())) ++RoomElitesSpawned;
	}

	if (RoomSpawned >= RoomQuota && RoomElitesSpawned >= RoomEliteCount)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle); // 모두 스폰됨; 필드가 비면 방 종료
	}
}

void AMLRunGameMode::EndRoom()
{
	if (RunState != EMLRunState::Combat) return; // 영웅 사망, 또는 같은 프레임의 두 번째 지연 호출
	
	// ClearAll 전에: 정예 방은 엘리트가 모두 죽었을 때만 승리로 침 (시간 초과는 제외)
	if (CurrentRoomType == EMLRoomType::Elite && IsRoomCleared()) ++EliteRoomsCleared;
	
	CloseRoomDamage(); // SetRunState(Collecting) 전에: HUD 패널이 그 브로드캐스트에서 읽음

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(RoomTimerHandle);
	GetWorldTimerManager().ClearTimer(EliteTimerHandle);
	GetWorldTimerManager().ClearTimer(PursuerTimerHandle);

	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	UMLPickupSubsystem* PickupSub = GetWorld()->GetSubsystem<UMLPickupSubsystem>();
	if (EnemySub) EnemySub->ClearAll(); // 황금 / 정예 방 시간 초과: 남은 적은 드롭 없이 사라짐
	EndGhost();
	const int32 Attracted = PickupSub ? PickupSub->AttractAll() : 0;

	SetRunState(EMLRunState::Collecting);
	if (Attracted == 0)
	{
		HandleGemsCollected(); // 바닥에 아무것도 없음
	}
	// 아니면: 마지막 보석 / 금화가 도착할 때 HandleGemsCollected가 이어감
}

void AMLRunGameMode::HandleGemsCollected()
{
	if (RunState != EMLRunState::Collecting) return;

	// design §6.2.3: 강적을 이겼으면 장비를 고른 뒤 포털이 열림
	const AMLHeroCharacter* Hero = GetHero();
	UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr;
	if (bGhostDefeated && RunProgress)
	{
		bGhostDefeated = false;
		RunProgress->OnRestFinished.RemoveAll(this);
		RunProgress->OnRestFinished.AddUObject(this, &AMLRunGameMode::HandleGraveChosen);
		RunProgress->OpenGraveChoices(RoomGrave.Items);
		return;
	}
	OpenGates();
}

void AMLRunGameMode::HandleGraveChosen()
{
	ClearedGraveMercId = RoomGrave.MercId; // 묘비는 장비를 고르는 순간 소모 (저장은 원정 종료 시)
	HandleSafeRoomFinished();
}

void AMLRunGameMode::OpenGates()
{
	const bool bLastRoom = IsLastRoomOfZone();
	TArray<EMLRoomType> PortalTypes;
	if (bLastRoom && IsLastZone())
	{
		PortalTypes.Add(EMLRoomType::Boss);
	}
	else
	{
		RollPortalTypes(bLastRoom, PortalTypes);
	}
	SpawnGates(PortalTypes, bLastRoom);

	ChannelGate = nullptr;
	ChannelSeconds = 0.f;
	GetWorldTimerManager().SetTimer(GateCheckTimerHandle, this, &AMLRunGameMode::GateCheckTick, GateCheckInterval, true);
	
	SetRunState(EMLRunState::Choosing);
}

void AMLRunGameMode::RollPortalTypes(bool bNextIsZoneStart, TArray<EMLRoomType>& OutTypes)
{
	const bool bAfterSafeRoom = CurrentRoomType == EMLRoomType::Rest || CurrentRoomType == EMLRoomType::Shop;

	TArray<EMLRoomType> Pool;
	TArray<float> Weights;
	for (const TPair<EMLRoomType, float>& Pair : RoomWeights)
	{
		const EMLRoomType Type = Pair.Key;
		const bool bSafeRoom = Type == EMLRoomType::Rest || Type == EMLRoomType::Shop;
		if (Pair.Value <= 0.f || Type == EMLRoomType::Boss) continue;
		if (bNextIsZoneStart && bSafeRoom) continue; // 구역의 첫 방은 전투 방
		if (bAfterSafeRoom && bSafeRoom) continue;   // 안전 방이 연속으로 나오지 않음
		Pool.Add(Type);
		Weights.Add(Pair.Value);
	}

	while (OutTypes.Num() < PortalCount && Pool.Num() > 0)
	{
		float Total = 0.f;
		for (const float W : Weights) Total += W;

		float Roll = FMath::FRandRange(0.f, Total);
		int32 Pick = Pool.Num() - 1;
		for (int32 i = 0; i < Weights.Num(); ++i)
		{
			Roll -= Weights[i];
			if (Roll <= 0.f)
			{
				Pick = i;
				break;
			}
		}
		OutTypes.Add(Pool[Pick]);
		Pool.RemoveAt(Pick);
		Weights.RemoveAt(Pick);
	}
}

void AMLRunGameMode::SpawnGates(const TArray<EMLRoomType>& PortalTypes, bool bWithExitDoor)
{
	const AMLHeroCharacter* Hero = GetHero();
	FVector HeroFloor = ArenaCenter;
	if (Hero)
	{
		HeroFloor = Hero->GetActorLocation();
		HeroFloor.Z -= Hero->GetSimpleCollisionHalfHeight();
	}

	const FRotator Facing(0.f, GateFacingYaw, 0.f);
	const FVector Forward = Facing.Vector();                              // 화면 위쪽
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);  // 화면 오른쪽
	const float MidIndex = (PortalTypes.Num() - 1) * 0.5f;

	// 영웅 주변 배치: 포털은 화면 위쪽, 탈출 문은 포털 줄 오른쪽.
	TArray<FVector> Offsets;
	for (int32 i = 0; i < PortalTypes.Num(); ++i)
	{
		Offsets.Add(Forward * PortalDistance + Right * ((i - MidIndex) * PortalSpacing));
	}
	const FVector DoorOffset = Forward * ExitDoorOffset.X + Right * ExitDoorOffset.Y;
	if (bWithExitDoor) Offsets.Add(DoorOffset);

	// 포털 하나하나가 아니라 배치 전체를 옮겨서 모든 포털이 경기장 바닥 안에 있게 함.
	FVector Anchor = HeroFloor;
	if (Offsets.Num() > 0)
	{
		const FBox Bounds(Offsets);
		Anchor.X = FMath::Clamp(Anchor.X, ArenaCenter.X - ArenaHalfSize - Bounds.Min.X, ArenaCenter.X + ArenaHalfSize - Bounds.Max.X);
		Anchor.Y = FMath::Clamp(Anchor.Y, ArenaCenter.Y - ArenaHalfSize - Bounds.Min.Y, ArenaCenter.Y + ArenaHalfSize - Bounds.Max.Y);
	}

	for (int32 i = 0; i < PortalTypes.Num(); ++i)
	{
		SpawnGate(PortalClass, Anchor + Offsets[i], PortalTypes[i], false);
	}
	PortalGroupCenter = Anchor + Forward * PortalDistance;

	bExitDoorOpen = bWithExitDoor && SpawnGate(ExitDoorClass, Anchor + DoorOffset, EMLRoomType::Normal, true) != nullptr;
}

AMLRoomGateActor* AMLRunGameMode::SpawnGate(TSubclassOf<AMLRoomGateActor> GateClass, const FVector& Location, EMLRoomType GateRoomType, bool bGateIsExitDoor)
{
	if (!GateClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] RunGameMode: %s class not set"), bGateIsExitDoor ? TEXT("ExitDoor") : TEXT("Portal"));
		return nullptr;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AMLRoomGateActor* Gate = GetWorld()->SpawnActor<AMLRoomGateActor>(GateClass, Location, FRotator::ZeroRotator, SpawnInfo);
	if (Gate)
	{
		Gate->InitGate(GateRoomType, bGateIsExitDoor);
		ActiveGates.Add(Gate);
	}
	return Gate;
}

void AMLRunGameMode::GateCheckTick()
{
	const AMLHeroCharacter* Hero = GetHero();
	if (!Hero || Hero->IsDead()) return;

	const FVector HeroLoc = Hero->GetActorLocation();
	AMLRoomGateActor* Inside = nullptr;
	float BestDistSq = FMath::Square(GateRadius);
	for (AMLRoomGateActor* Gate : ActiveGates)
	{
		if (!Gate) continue;
		const float DistSq = FVector::DistSquared2D(HeroLoc, Gate->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Inside = Gate;
		}
	}

	if (Inside != ChannelGate)
	{
		if (ChannelGate)
		{
			ChannelGate->SetChannelProgress(0.f);
		}
		ChannelGate = Inside;
		ChannelSeconds = 0.f;
	}
	if (!Inside) return;

	ChannelSeconds += GateCheckInterval;
	const float Required = Inside->IsExitDoor() ? ExitDoorChooseSeconds : PortalChooseSeconds;
	Inside->SetChannelProgress(ChannelSeconds / Required);
	if (ChannelSeconds + KINDA_SMALL_NUMBER >= Required)
	{
		ChooseGate(Inside);
	}
}

void AMLRunGameMode::ChooseGate(AMLRoomGateActor* Gate)
{
	GetWorldTimerManager().ClearTimer(GateCheckTimerHandle);

	if (Gate->IsExitDoor())
	{
		FinishRun(true);
		return;
	}
	
	BeginTransition(Gate->GetRoomType());
}

void AMLRunGameMode::BeginTransition(EMLRoomType NextRoomType)
{
	PendingRoomType = NextRoomType;
	SetRunState(EMLRunState::Transition);

	if (const APlayerController* PC = GetHeroPC(); PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, FadeSeconds, FLinearColor::Black, false, true);
	}
	GetWorldTimerManager().SetTimer(TransitionTimerHandle, this, &AMLRunGameMode::CompleteTransition, FadeSeconds, false);
}

void AMLRunGameMode::CompleteTransition()
{
	ClearGates();

	if (PendingRoomType == EMLRoomType::Boss)
	{
		// 보스 방은 3구역 마지막 방 다음: 결과 화면이 원정이 끝난 곳을 보여주도록 구역 / 방 유지
	}
	else if (IsLastRoomOfZone())
	{
		++CurrentZone;
		CurrentRoom = 1;
	}
	else
	{
		++CurrentRoom;
	}

	if (AMLHeroCharacter* Hero = GetHero())
	{
		Hero->SetActorLocation(ArenaCenter, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (const APlayerController* PC = GetHeroPC(); PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeSeconds, FLinearColor::Black);
	}

	StartRoom(PendingRoomType);
}

void AMLRunGameMode::ClearGates()
{
	for (AMLRoomGateActor* Gate : ActiveGates)
	{
		if (Gate) Gate->Destroy();
	}
	ActiveGates.Reset();
	ChannelGate = nullptr;
	bExitDoorOpen = false;
}

void AMLRunGameMode::FinishRun(bool bExtracted)
{
	if (RunState == EMLRunState::Finished) return; // 사망 vs 탈출: 먼저 온 쪽이 우선

	GetWorldTimerManager().ClearAllTimersForObject(this);
	CloseRoomDamage(); // 방 도중 사망 / 보스 방: 그 방도 집계함
	SetRunState(EMLRunState::Finished);

	const AMLHeroCharacter* Hero = GetHero();
	const UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr;

	FMLRunSummary Summary;
	Summary.bExtracted = bExtracted;
	Summary.Zone = CurrentZone;
	Summary.Room = CurrentRoom;
	Summary.TimeSeconds = GetWorld()->GetTimeSeconds() - RunStartSeconds;
	Summary.Kills = KillCount;
	Summary.RunLevel = RunProgress ? RunProgress->GetRunLevel() : 1;
	Summary.Gold = GetRunGold();
	Summary.bBossKilled = bBossDefeated;
	
	MakeDamageLines(RunDamage, Summary.DamageLines);
	Summary.CombatSeconds = RunCombatSeconds;

	if (UMLGuildSubsystem* Guild = GetGuild())
	{
		FMLRunReport Report;
		Report.bExtracted = bExtracted;
		Report.bBossKilled = bBossDefeated;
		Report.Zone = CurrentZone;
		Report.Kills = KillCount;
		Report.EliteKills = EliteKillCount;
		Report.Gold = Summary.Gold;
		Report.RewardMult = GetZoneSpawn().GoldMult; // design §7.1.1: 도달 구역의 보상 배율
		if (RunProgress) Report.Items = RunProgress->GetRunItems();
		Report.GoldRoomsCleared = GoldRoomsCleared;
		Report.EliteRoomsCleared = EliteRoomsCleared;
		Report.Greed = GetGreedLevel();
		Report.ClearedGraveMercId = ClearedGraveMercId;
		if (RunProgress)
		{
			for (const TObjectPtr<UMLAutoSkill>& Skill : RunProgress->GetOwnedSkills())
			{
				if (Skill) Report.Skills.Add(FSoftClassPath(Skill->GetClass()));
			}
			RunProgress->GetPassiveLevels().GetKeys(Report.Passives);
		}
		Summary.Reward = Guild->EndRun(Report);
		if (bExtracted) Summary.Gold = Summary.Reward.GoldAdded; // 결과 화면은 실제로 길드에 들어간 값을 보여줌 (가문 보너스)
	}

	APlayerController* PC = GetHeroPC();
	if (!PC || !ResultWidgetClass) return;

	ResultWidget = CreateWidget<UMLRunResultWidget>(PC, ResultWidgetClass);
	if (!ResultWidget) return;

	ResultWidget->Setup(Summary);
	ResultWidget->AddToViewport(20);
	PC->SetInputMode(FInputModeUIOnly());
	PC->SetShowMouseCursor(true);
	UGameplayStatics::SetGamePaused(this, true);
}

void AMLRunGameMode::HandleEnemyKilled(const AMLEnemyActor* Killed, bool bElite)
{
	++KillCount;
	if (bElite) ++EliteKillCount;
	if (RunState != EMLRunState::Combat) return;
	// design §6.3: 엘리트 상자는 이 구역의 드롭 단계 사용
	if (bElite && Killed) SpawnChest(Killed->GetActorLocation(), CurrentZone);

	if (GhostActor && Killed == GhostActor.Get()) // 처치 할당량에는 포함되지 않음
	{
		bGhostDefeated = true;
		EndGhost();
		OnRoomStateChanged.Broadcast();
		if (IsRoomCleared()) GetWorldTimerManager().SetTimerForNextTick(this, &AMLRunGameMode::EndRoom);
		return;
	}

	++RoomKills;
	if (bElite) ++RoomElitesKilled;

	if (bGhostPending && !bGhostWarned && RoomKills >= FMath::CeilToInt(RoomQuota * GhostKillStep))
	{
		bGhostWarned = true; // 아래 브로드캐스트에서 HUD가 "강적 등장!" 표시
		GetWorldTimerManager().SetTimer(GhostTimerHandle, this, &AMLRunGameMode::SpawnGhost, GhostWarnSeconds, false);
	}

	OnRoomStateChanged.Broadcast(); // HUD "남은 적" / "남은 엘리트"
	
	if (CurrentRoomType == EMLRoomType::Boss)
	{
		if (BossActor && Killed == BossActor.Get())
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AMLRunGameMode::HandleBossKilled); // 피해 루프 밖에서
		}
		return; // 소환된 박쥐는 보스 방을 끝내지 않음
	}

	// 적 피해 루프 안에서 호출됨; 방은 다음 틱에 끝냄.
	if (CurrentRoomType == EMLRoomType::Gold && GoldGoblinClass && Killed && Killed->IsA(GoldGoblinClass))
	{
		++GoldRoomsCleared;
		SpawnChest(Killed->GetActorLocation(), FMath::Min(CurrentZone + 1, BossDropStep)); // design §6.2.1: 슬라임 = 한 단계 위
		GetWorldTimerManager().SetTimerForNextTick(this, &AMLRunGameMode::EndRoom);
		return;
	}
	if (IsRoomCleared())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AMLRunGameMode::EndRoom);
	}
}

bool AMLRunGameMode::IsRoomCleared() const
{
	const bool bAllElitesOut = RoomElitesSpawned >= RoomEliteCount && !bPursuerPending && !bGhostPending;
	if (CurrentRoomType == EMLRoomType::Elite)
	{
		return bAllElitesOut && GetElitesLeft() == 0; // 그곳의 일반 적은 목표에 포함되지 않음
	}

	const UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	return bAllElitesOut && RoomSpawned >= RoomQuota && EnemySub && EnemySub->GetActiveCount() == 0; // 황금 방: 할당량이 MAX이므로 절대 true가 아님
}

// [MLRunGameMode.cpp] 파일 끝에 추가
int32 AMLRunGameMode::GetEnemiesLeft() const
{
	if (RoomQuota == MAX_int32) return 0; // 황금 / 정예 방은 처치 할당량이 없음
	return FMath::Max(0, RoomQuota + GetRoomEliteTotal() - RoomKills);
}

bool AMLRunGameMode::GetEliteLocation(FVector& OutLocation) const
{
	if (RunState != EMLRunState::Combat || CurrentRoomType != EMLRoomType::Elite) return false;

	const AMLHeroCharacter* Hero = GetHero();
	const UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	const int32 Index = (Hero && EnemySub) ? EnemySub->FindNearestElite(Hero->GetActorLocation()) : INDEX_NONE;
	if (Index == INDEX_NONE) return false;

	OutLocation = EnemySub->GetEnemyLocation(Index);
	return true;
}

bool AMLRunGameMode::GetGhostLocation(FVector& OutLocation) const
{
	const UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	if (RunState != EMLRunState::Combat || !GhostActor || !EnemySub || !EnemySub->IsEnemyActive(GhostActor->GetEnemyIndex())) return false;

	OutLocation = GhostActor->GetActorLocation();
	return true;
}

FText AMLRunGameMode::GetGhostName() const
{
	return FText::Format(NSLOCTEXT("ML", "GhostName", "망령 {0}"), FText::FromString(RoomGrave.Name));
}

void AMLRunGameMode::SetRunState(EMLRunState NewState)
{
	RunState = NewState;
	OnRoomStateChanged.Broadcast();
}

void AMLRunGameMode::CloseRoomDamage()
{
	if (!bRoomDamageOpen) return;
	bRoomDamageOpen = false;

	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	if (!EnemySub) return;

	LastRoomCombatSeconds = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - RoomCombatStartSeconds); // 월드 시간: 일시정지 제외
	RunCombatSeconds += LastRoomCombatSeconds;

	const TMap<FName, float> RoomDamage = EnemySub->TakeRoomDamage();
	for (const TPair<FName, float>& Pair : RoomDamage)
	{
		RunDamage.FindOrAdd(Pair.Key) += Pair.Value;
	}
	MakeDamageLines(RoomDamage, LastRoomDamage);
}

void AMLRunGameMode::MakeDamageLines(const TMap<FName, float>& Damage, TArray<FMLDamageLine>& OutLines) const
{
	OutLines.Reset();
	const UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	for (const TPair<FName, float>& Pair : Damage)
	{
		if (Pair.Value <= 0.f) continue;
		FMLDamageLine& Line = OutLines.AddDefaulted_GetRef();
		Line.Name = EnemySub ? EnemySub->GetDamageSourceName(Pair.Key) : FText::FromName(Pair.Key);
		Line.Damage = Pair.Value;
	}
	OutLines.Sort([](const FMLDamageLine& A, const FMLDamageLine& B) { return A.Damage > B.Damage; });
}

FMLZoneSpawnConfig AMLRunGameMode::GetZoneSpawn() const
{
	FMLZoneSpawnConfig Config = ZoneSpawns.IsValidIndex(CurrentZone - 1) ? ZoneSpawns[CurrentZone - 1] : FMLZoneSpawnConfig();
	Config.HealthMult *= EnemyHealthScale; // design §6.5 (피해는 StartRoom에서 따로 곱함)
	return Config;
}

bool AMLRunGameMode::IsLastRoomOfZone() const
{
	return CurrentRoom >= GetRoomsInZone();
}

bool AMLRunGameMode::IsLastZone() const
{
	return CurrentZone >= RoomsPerZone.Num();
}

AMLHeroCharacter* AMLRunGameMode::GetHero() const
{
	return Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

APlayerController* AMLRunGameMode::GetHeroPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

int32 AMLRunGameMode::GetRunGold() const
{
	const AMLHeroCharacter* Hero = GetHero();
	const UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr;
	return RunProgress ? RunProgress->GetRunGold() : 0;
}

UMLGuildSubsystem* AMLRunGameMode::GetGuild() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UMLGuildSubsystem>() : nullptr;
}

int32 AMLRunGameMode::GetGreedLevel() const
{
	const AMLHeroCharacter* Hero = GetHero();
	const UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr;
	// design §6.2.2: 미확보 가치 = 원정 골드 + 미확보 아이템당 Tier × ItemGreedGoldPerTier
	const int32 Unsecured = RunProgress ? RunProgress->GetRunGold() + RunProgress->GetRunItemTierTotal() * ItemGreedGoldPerTier : 0;
	int32 Level = 0;
	for (const int32 Threshold : GreedThresholds)
	{
		if (Unsecured >= Threshold) ++Level;
	}
	return Level;
}

bool AMLRunGameMode::GetGoblinLocation(FVector& OutLocation) const
{
	if (RunState != EMLRunState::Combat || CurrentRoomType != EMLRoomType::Gold) return false;

	const UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	const int32 Index = EnemySub ? EnemySub->FindFleeingEnemy() : INDEX_NONE;
	if (Index == INDEX_NONE) return false;

	OutLocation = EnemySub->GetEnemyLocation(Index);
	return true;
}

void AMLRunGameMode::StartBossRoom()
{
	RoomQuota = MAX_int32; // 처치 할당량 없음: 보스가 목표
	RoomSpeedMult = 1.f;   // design §6.5: 탐욕은 보스 방에 영향을 주지 않음
	BossFightSeconds = 0;
	bBossDefeated = false;

	const AMLHeroCharacter* Hero = GetHero();
	UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	const FVector ScreenUp = FRotator(0.f, GateFacingYaw, 0.f).Vector();
	int32 BossIndex = INDEX_NONE;
	if (!BossClass || !Hero || !EnemySub
		|| !EnemySub->SpawnEnemy(BossClass, Hero->GetActorLocation() + ScreenUp * BossSpawnDistance, EnemyHealthScale, 1.f, 1.f, EMLEliteTrait::None, &BossIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] RunGameMode: boss could not spawn (BossClass set?) - counts as cleared"));
		FinishRun(true);
		return;
	}

	BossActor = Cast<AMLBossActor>(EnemySub->GetEnemyActor(BossIndex));
	if (BossActor) BossActor->StartBoss(BossIndex);

	GetWorldTimerManager().SetTimer(RoomTimerHandle, this, &AMLRunGameMode::BossSecondTick, 1.f, true);
	SetRunState(EMLRunState::Combat);
}

void AMLRunGameMode::BossSecondTick()
{
	++BossFightSeconds;
	OnRoomStateChanged.Broadcast(); // HUD 전투 타이머
}

void AMLRunGameMode::HandleBossKilled()
{
	if (bBossDefeated || RunState != EMLRunState::Combat) return;
	bBossDefeated = true;

	GetWorldTimerManager().ClearTimer(RoomTimerHandle);
	if (BossActor) BossActor->StopBoss();
	if (UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>())
	{
		EnemySub->ClearAll(); // 소환된 박쥐는 사라짐
	}

	// design §6.2: 보스 처치 = 강제 탈출 + 큰 보상, 즉시 지급 (금화를 주울 시간이 없음)
	AMLHeroCharacter* Hero = GetHero();
	if (UMLRunProgressComponent* RunProgress = Hero ? Hero->GetRunProgress() : nullptr)
	{
		RunProgress->AddGold(BossGoldReward);
		RunProgress->AddChest(BossDropStep); // design §6.3: 보스 아이템도 골드처럼 즉시 지급
	}

	OnRoomStateChanged.Broadcast(); // HUD "보스 처치!"
	GetWorldTimerManager().SetTimer(TransitionTimerHandle, FTimerDelegate::CreateUObject(this, &AMLRunGameMode::FinishRun, true), BossVictorySeconds, false);
}

bool AMLRunGameMode::GetBossHealth(float& OutPercent, FText& OutName) const
{
	const UMLEnemySubsystem* EnemySub = GetWorld()->GetSubsystem<UMLEnemySubsystem>();
	if (GhostActor)
	{
		OutPercent = EnemySub ? EnemySub->GetEnemyHealthPercent(GhostActor->GetEnemyIndex()) : 0.f;
		OutName = GetGhostName();
		return true;
	}
	if (CurrentRoomType != EMLRoomType::Boss || !BossActor) return false;

	OutPercent = EnemySub ? EnemySub->GetEnemyHealthPercent(BossActor->GetEnemyIndex()) : 0.f;
	OutName = BossActor->GetBossName();
	return true;
}

void AMLRunGameMode::SpawnChest(const FVector& Location, int32 Step)
{
	UMLPickupSubsystem* PickupSub = GetWorld()->GetSubsystem<UMLPickupSubsystem>();
	if (!ItemChestClass || !PickupSub)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] RunGameMode: ItemChestClass not set, no item chest"));
		return;
	}
	PickupSub->SpawnGem(ItemChestClass, Location, static_cast<float>(Step));
}
