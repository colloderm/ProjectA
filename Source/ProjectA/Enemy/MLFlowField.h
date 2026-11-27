// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"

class UWorld;

// design §6.5 적 이동 (M9-3d): 영웅을 향한 흐름장. 영웅 주변을 칸으로 나눠 길을 한 번 계산하고 모든 적이 같이 씀.
// 맵에 장애물이 없으면 계산을 건너뛰고 지금처럼 직선으로 감.
// shortcut: 막힘은 칸마다 한 번만 검사해 기억함(정적 지형 전용). 움직이는 장애물이 생기면 캐시를 지우는 함수 추가
struct FMLFlowField
{
	// 영웅의 칸이 바뀌었거나 RebuildInterval이 지났으면 다시 계산
	void Update(UWorld* World, const FVector& HeroLoc, float DeltaTime);

	// From에서 영웅 쪽으로 갈 2D 방향. 흐름장 밖이거나 영웅 근처면 직선
	FVector GetMoveDir(const FVector& From, const FVector& HeroLoc) const;

	// 이동 결과가 막힌 칸이면 X만 → Y만 → 제자리 순서로 벽을 따라 미끄러짐
	FVector Slide(UWorld* World, const FVector& From, const FVector& To);

	bool IsBlocked(UWorld* World, const FVector& Loc);

	// Desired가 막혔으면 가까운 빈 칸 (스폰 위치)
	FVector FindOpenLocation(UWorld* World, const FVector& Desired);

private:
	FIntPoint ToCell(const FVector& Loc) const;
	bool IsCellBlocked(UWorld* World, const FIntPoint& Cell, float Z);
	void Build(UWorld* World, float Z);
	int32 ToIndex(const FIntPoint& Cell) const; // 창 밖이면 INDEX_NONE

	TMap<FIntPoint, bool> BlockedCache;
	TArray<int32> Distance; // 창 안 칸별 영웅까지의 칸 수, -1 = 막힘 / 못 감
	FIntPoint Origin = FIntPoint::ZeroValue; // 영웅의 칸
	float RebuildTimer = 0.f;
	bool bBuilt = false;
	bool bHasObstacles = false;

	static constexpr float CellSize = 100.f;
	static constexpr int32 Radius = 30;               // 창 = (2R+1)² 칸, 반경 3000
	static constexpr float RebuildInterval = 0.25f;
	static constexpr float CheckHalfXY = 60.f;         // 칸 반폭 50 + 여유
	static constexpr float CheckHalfZ = 40.f;          // 영웅 캡슐 중심 높이에서: 바닥과 낮은 턱은 막힘이 아님
	static constexpr int32 DirectCells = 2;            // 영웅과 이만큼 가까우면 직선
	static constexpr int32 OpenSearchRings = 10;
};
