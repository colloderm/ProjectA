// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Run/MLRunTypes.h"
#include "MLRunGameMode.generated.h"

class AMLHeroCharacter;
class AMLEnemyActor;
class AMLBossActor;
class AMLGhostActor;
class AMLRoomGateActor;
class AMLChoiceArea;
class APlayerController;
class UMLRunResultWidget;
class UMLGuildSubsystem;
class AMLPickupActor;
enum class EMLEliteTrait : uint8;

DECLARE_MULTICAST_DELEGATE(FMLOnRoomStateChanged);

// design §6.2: 구역마다 여러 방; 방 종료 -> 포털 3개 (+ 구역 끝이면 탈출 문).
UCLASS()
class PROJECTA_API AMLRunGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMLRunGameMode();

	// AMLHeroCharacter::HandleDeath가 한 번 호출
	void HandleHeroDied();

	EMLRunState GetRunState() const { return RunState; }
	EMLRoomType GetCurrentRoomType() const { return CurrentRoomType; }
	int32 GetZone() const { return CurrentZone; }
	int32 GetRoomInZone() const { return CurrentRoom; }
	int32 GetRoomsInZone() const;
	int32 GetEnemiesLeft() const;
	FVector GetPortalGroupCenter() const { return PortalGroupCenter; }
	bool IsExitDoorOpen() const { return bExitDoorOpen; }
	int32 GetRoomSecondsLeft() const { return RoomSecondsLeft; } // 황금 방 카운트다운
	int32 GetGreedLevel() const;
	int32 GetMaxGreedLevel() const { return GreedThresholds.Num(); }
	
	// 황금 방 전용: 슬라임이 살아 있는 동안 true (HUD 화살표)
	bool GetGoblinLocation(FVector& OutLocation) const;
	int32 GetElitesLeft() const { return FMath::Max(0, GetRoomEliteTotal() - RoomElitesKilled); } // 정예 방 HUD
	// 정예 방 전용: 가장 가까운 살아 있는 엘리트 (HUD 화살표)
	bool GetEliteLocation(FVector& OutLocation) const;
	// 강적(망령)이 살아 있는 동안 true (HUD 화살표)
	bool GetGhostLocation(FVector& OutLocation) const;
	// "강적 등장!" 안내 중 (나타나기 전 GhostWarnSeconds 동안)
	bool IsGhostWarning() const { return bGhostPending && bGhostWarned; }
	FText GetGhostName() const;
	int32 GetGhostSecondsLeft() const { return GhostActor ? GhostSecondsLeft : 0; }
	// 보스 패널 HUD (보스 · 망령): 둘 다 없으면 false
	bool GetBossHealth(float& OutPercent, FText& OutName) const;
	int32 GetBossFightSeconds() const { return BossFightSeconds; }
	bool IsBossDefeated() const { return bBossDefeated; }
	
	// design §6 HUD 피해 리포트 (M9-0): 마지막으로 끝난 전투 방; 안전 방 / 전투 중에는 비어 있음
	const TArray<FMLDamageLine>& GetRoomDamageLines() const { return LastRoomDamage; }
	float GetRoomCombatSeconds() const { return LastRoomCombatSeconds; }

	FMLOnRoomStateChanged OnRoomStateChanged;

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;

private:
	void StartRoom(EMLRoomType NewRoomType);
	void StartGoldRoom();
	// 강적 등장 (design §6.2.3)
	void RollGhost();
	void SpawnGhost();
	void GhostSecondTick();
	void EndGhost(); // 처치 / 퇴각 / 방 종료: 타이머와 액터 정리
	void HandleGraveChosen();
	void StartBossRoom();
	void BossSecondTick();
	void HandleBossKilled();
	void StartEliteRoom();
	void EliteRoomSpawnTick();
	void SpawnPursuer();
	bool SpawnElite(const FVector& Location, EMLEliteTrait Trait);
	bool IsRoomCleared() const;
	FVector GetRingLocation(const FVector& HeroLoc) const;
	int32 GetRoomEliteTotal() const { return RoomEliteCount + (bRoomHasPursuer ? 1 : 0); }
	void RoomSecondTick();
	void StartSafeRoom();
	void ShowSafeRoomUI();
	void HandleSafeRoomFinished();
	void SpawnTick();
	void EndRoom();
	void HandleGemsCollected();
	void OpenGates();
	void ArriveAtChoiceArea(); // design §6.2 선택 구역 (M9-3e)
	void RollPortalTypes(bool bNextIsZoneStart, TArray<EMLRoomType>& OutTypes);
	void SpawnGates(const TArray<EMLRoomType>& PortalTypes, bool bWithExitDoor);
	AMLRoomGateActor* SpawnGate(TSubclassOf<AMLRoomGateActor> GateClass, const FVector& Location, EMLRoomType GateRoomType, bool bGateIsExitDoor);
	void GateCheckTick();
	void ChooseGate(AMLRoomGateActor* Gate);
	void BeginTransition(EMLRoomType NextRoomType);
	void CompleteTransition();
	void ClearGates();
	void FinishRun(bool bExtracted);
	void HandleEnemyKilled(const AMLEnemyActor* Killed, bool bElite);
	void SpawnChest(const FVector& Location, int32 Step);
	void SetRunState(EMLRunState NewState);
	void CloseRoomDamage();
	void MakeDamageLines(const TMap<FName, float>& Damage, TArray<FMLDamageLine>& OutLines) const;

	FMLZoneSpawnConfig GetZoneSpawn() const;
	bool IsLastRoomOfZone() const;
	bool IsLastZone() const;
	AMLHeroCharacter* GetHero() const;
	APlayerController* GetHeroPC() const;
	int32 GetRunGold() const;
	UMLGuildSubsystem* GetGuild() const;
	bool HasPersonality(EMLPersonality Personality) const; // design §7.3 (M9-4)

	// --- 구조 (design §6.2) ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|Structure")
	TArray<int32> RoomsPerZone;

	// 인덱스 = 구역 - 1
	UPROPERTY(EditDefaultsOnly, Category = "Run|Spawn")
	TArray<FMLZoneSpawnConfig> ZoneSpawns;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Spawn")
	float SpawnRingMin = 1400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Spawn")
	float SpawnRingMax = 1800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Rooms")
	TMap<EMLRoomType, float> RoomWeights;

	// design §7.3 성격 (M9-4): 탐욕가·추적자·순례자가 좋아하는 방의 포털 가중치 배율
	UPROPERTY(EditDefaultsOnly, Category = "Run|Personality", meta = (ClampMin = "1"))
	float PersonalityWeightMult = 2.f;

	// 탐욕가: 탐욕 단계를 정하는 미확보 가치에 곱함 (1.1 = 10% 빨리 오름)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Personality", meta = (ClampMin = "1"))
	float GreedyGreedMult = 1.1f;
	
	// --- 탐욕 (design §6.2.2) ---
	// 탐욕 1..N에 필요한 원정 골드 (오름차순). 방이 시작될 때마다 한 번 읽음.
	UPROPERTY(EditDefaultsOnly, Category = "Run|Greed")
	TArray<int32> GreedThresholds;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Greed")
	float GreedSpawnPerLevel = 0.15f; // 레벨당 방 처치 할당량 +15%

	UPROPERTY(EditDefaultsOnly, Category = "Run|Greed")
	float GreedSpeedPerLevel = 0.05f; // 레벨당 적 이동 속도 +5%

	// --- 황금 방 (design §6.2.1) ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|GoldRoom")
	TSubclassOf<AMLEnemyActor> GoldGoblinClass;

	UPROPERTY(EditDefaultsOnly, Category = "Run|GoldRoom", meta = (ClampMin = "5"))
	float GoldRoomSeconds = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|GoldRoom", meta = (ClampMin = "0"))
	int32 GoldRoomMaxAlive = 30;

	// 슬라임은 영웅에서 화면 위쪽(GateFacingYaw)으로 이만큼 떨어져 나타나므로 방 시작 시 항상 화면에 보임
	UPROPERTY(EditDefaultsOnly, Category = "Run|GoldRoom")
	float GoblinSpawnDistance = 500.f;
	
	// --- 엘리트 (design §6.2.1 정예 방, §6.2.2 추격자) ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "10"))
	float EliteRoomSeconds = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "1"))
	float EliteRoomSpawnInterval = 8.f;

	// 엘리트와 함께 살아 있는 일반 적 (목표에 포함되지 않음)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "0"))
	int32 EliteRoomMaxAlive = 20;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "1"))
	float EliteRoomGoldMult = 1.5f;

	// 일반 방: 처치 수가 할당량 × 이 값 × k에 도달하면 k번째 엘리트 등장
	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "0.05", ClampMax = "0.33"))
	float EliteKillStep = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "1"))
	int32 PursuerGreedLevel = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Elite", meta = (ClampMin = "0"))
	float PursuerDelay = 10.f;
	
	// --- 강적 등장 (design §6.2.3) ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|Grave")
	TSubclassOf<AMLGhostActor> GhostClass;

	// 묘비가 있는 구역의 일반 방이 시작될 때마다; 정해지면 그 원정에서는 다시 나오지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Run|Grave", meta = (ClampMin = "0", ClampMax = "1"))
	float GraveChance = 0.3f;

	// 처치 할당량의 이 비율을 잡으면 "강적 등장!" 안내
	UPROPERTY(EditDefaultsOnly, Category = "Run|Grave", meta = (ClampMin = "0", ClampMax = "0.9"))
	float GhostKillStep = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Grave", meta = (ClampMin = "0.1"))
	float GhostWarnSeconds = 3.f;

	// 이 안에 못 잡으면 물러남 (묘비는 남음)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Grave", meta = (ClampMin = "5"))
	float GhostSeconds = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Grave")
	float GhostSpawnDistance = 1000.f;

	// --- 보스 (design §6.5) ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|Boss")
	TSubclassOf<AMLBossActor> BossClass;

	// 보스는 영웅에서 화면 위쪽으로 이만큼 떨어져 나타남
	UPROPERTY(EditDefaultsOnly, Category = "Run|Boss")
	float BossSpawnDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Boss", meta = (ClampMin = "0"))
	int32 BossGoldReward = 750;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Boss", meta = (ClampMin = "0"))
	float BossVictorySeconds = 2.f;
	
	// --- 아이템 (design §6.3) ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|Items")
	TSubclassOf<AMLPickupActor> ItemChestClass;

	// design §6.2.2: 미확보 아이템 하나는 탐욕 계산에서 Tier × 이 골드로 침
	UPROPERTY(EditDefaultsOnly, Category = "Run|Items", meta = (ClampMin = "0"))
	int32 ItemGreedGoldPerTier = 50;

	static constexpr int32 BossDropStep = 4;

	// --- 포털 ---
	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	TSubclassOf<AMLRoomGateActor> PortalClass;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	TSubclassOf<AMLRoomGateActor> ExitDoorClass;
	
	// 포털은 ArenaCenter ± 이 값(정사각형) 안에 둠. 경기장 맵의 바닥 크기에 맞출 것.
	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float ArenaHalfSize = 1800.f;

	// = 영웅 CameraBoom yaw, 그래서 "앞"이 화면 위쪽
	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float GateFacingYaw = 45.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float PortalDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float PortalSpacing = 450.f;

	// 영웅 기준 탈출 문 위치: X = 화면 위쪽, Y = 화면 오른쪽 (포털 줄의 오른쪽)
	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	FVector2D ExitDoorOffset = FVector2D(250.f, 900.f);

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float GateRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float PortalChooseSeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float ExitDoorChooseSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Run|Gates")
	float FadeSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMLRunResultWidget> ResultWidgetClass;

	// --- 런타임 ---
	UPROPERTY(Transient)
	TArray<TObjectPtr<AMLRoomGateActor>> ActiveGates;

	UPROPERTY(Transient)
	TObjectPtr<AMLRoomGateActor> ChannelGate;

	UPROPERTY(Transient)
	TObjectPtr<UMLRunResultWidget> ResultWidget;
	
	UPROPERTY(Transient)
	TObjectPtr<AMLBossActor> BossActor;

	UPROPERTY(Transient)
	TObjectPtr<AMLGhostActor> GhostActor;

	// 맵에 놓인 선택 구역, 없으면 nullptr (방이 끝난 자리 앞에 포털)
	UPROPERTY(Transient)
	TObjectPtr<AMLChoiceArea> ChoiceArea;

	bool bAtChoiceArea = false; // 영웅이 선택 구역에 있음 (포털 선택 중)

	FTimerHandle SpawnTimerHandle;
	FTimerHandle GateCheckTimerHandle;
	FTimerHandle TransitionTimerHandle;
	FTimerHandle RoomTimerHandle;
	FTimerHandle EliteTimerHandle;
	FTimerHandle PursuerTimerHandle;
	FTimerHandle GhostTimerHandle;						// 안내 → 등장 → 초 세기를 차례로 맡음

	FVector ArenaCenter = FVector::ZeroVector;
	EMLRunState RunState = EMLRunState::Transition;
	EMLRoomType CurrentRoomType = EMLRoomType::Normal;
	EMLRoomType PendingRoomType = EMLRoomType::Normal;
	int32 CurrentZone = 1;
	int32 CurrentRoom = 1;
	int32 KillCount = 0;
	int32 EliteKillCount = 0;							// design §7.1.1 용병 경험치
	int32 GoldRoomsCleared = 0;							// design §8.5.1 의뢰: 처치한 황금 슬라임
	int32 EliteRoomsCleared = 0;						// 승리한 정예 방 (시간 초과는 제외)
	float EnemyHealthScale = 1.f;						// design §6.5 용병 등급 보정 (적 HP와 영웅이 받는 피해)
	int32 RoomKills = 0;
	int32 RoomSpawned = 0;
	int32 RoomQuota = 0;
	int32 RoomSecondsLeft = 0;
	float RoomSpeedMult = 1.f;
	int32 RoomEliteCount = 0;							// 이번 방의 구역 엘리트 (추격자 제외)
	int32 RoomElitesSpawned = 0;						// RoomEliteCount 중
	int32 RoomElitesKilled = 0;							// 추격자 포함 모든 엘리트
	bool bRoomHasPursuer = false;
	bool bPursuerPending = false;						// 아직 스폰되지 않은 추격자가 있음
	bool bGhostOffered = false;							// 이번 원정에서 강적이 이미 정해짐
	bool bGhostPending = false;							// 이 방에 강적이 나올 예정, 아직 스폰 전
	bool bGhostWarned = false;							// 이 방에서 "강적 등장!" 안내를 시작함
	bool bGhostDefeated = false;						// 이 방의 망령을 잡음, 장비 선택을 아직 열지 않음
	int32 GhostSecondsLeft = 0;
	FMLGrave RoomGrave;									// 이 방의 묘비 (저장 데이터의 복사본)
	int32 ClearedGraveMercId = INDEX_NONE;				// 장비를 고른 묘비, 원정 종료 시 보고
	int32 BossFightSeconds = 0;
	bool bBossDefeated = false;
	FVector PortalGroupCenter = FVector::ZeroVector;
	float ChannelSeconds = 0.f;
	float RunStartSeconds = 0.f;
	bool bExitDoorOpen = false;
	
	// design §6 HUD 피해 리포트 (M9-0)
	bool bRoomDamageOpen = false;						// 전투 방을 집계 중
	float RoomCombatStartSeconds = 0.f;
	float LastRoomCombatSeconds = 0.f;
	TArray<FMLDamageLine> LastRoomDamage;
	TMap<FName, float> RunDamage;
	float RunCombatSeconds = 0.f;

	static constexpr int32 PortalCount = 3;
	static constexpr float GateCheckInterval = 0.1f;
};
