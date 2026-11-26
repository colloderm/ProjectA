// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLFlowField.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

namespace
{
	constexpr int32 WindowSize(int32 R) { return 2 * R + 1; }
	const FIntPoint Neighbors[8] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
}

FIntPoint FMLFlowField::ToCell(const FVector& Loc) const
{
	return FIntPoint(FMath::FloorToInt(Loc.X / CellSize), FMath::FloorToInt(Loc.Y / CellSize));
}

int32 FMLFlowField::ToIndex(const FIntPoint& Cell) const
{
	const int32 LX = Cell.X - Origin.X + Radius;
	const int32 LY = Cell.Y - Origin.Y + Radius;
	const int32 Size = WindowSize(Radius);
	return (LX >= 0 && LX < Size && LY >= 0 && LY < Size) ? LY * Size + LX : INDEX_NONE;
}

bool FMLFlowField::IsCellBlocked(UWorld* World, const FIntPoint& Cell, float Z)
{
	if (const bool* Cached = BlockedCache.Find(Cell)) return *Cached;
	if (!World) return false;

	// 지형만: 영웅(Pawn)은 대상이 아니고, 적·탄·포털·기술 표시는 충돌이 꺼져 있음
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FVector Center((Cell.X + 0.5f) * CellSize, (Cell.Y + 0.5f) * CellSize, Z);
	const bool bBlocked = World->OverlapAnyTestByObjectType(Center, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeBox(FVector(CheckHalfXY, CheckHalfXY, CheckHalfZ)), FCollisionQueryParams(SCENE_QUERY_STAT(MLFlowField), false));
	BlockedCache.Add(Cell, bBlocked);
	return bBlocked;
}

bool FMLFlowField::IsBlocked(UWorld* World, const FVector& Loc)
{
	return IsCellBlocked(World, ToCell(Loc), Loc.Z);
}

void FMLFlowField::Update(UWorld* World, const FVector& HeroLoc, float DeltaTime)
{
	RebuildTimer -= DeltaTime;
	const FIntPoint HeroCell = ToCell(HeroLoc);
	if (bBuilt && HeroCell == Origin && RebuildTimer > 0.f) return;

	Origin = HeroCell;
	RebuildTimer = RebuildInterval;
	Build(World, HeroLoc.Z);
}

void FMLFlowField::Build(UWorld* World, float Z)
{
	const double StartSeconds = FPlatformTime::Seconds();
	const int32 Size = WindowSize(Radius);
	Distance.Init(-1, Size * Size);
	bBuilt = true;

	// 막힘 표시 (처음 보는 칸만 실제 검사)
	TArray<bool> Blocked;
	Blocked.Init(false, Size * Size);
	int32 BlockedCount = 0;
	for (int32 LY = 0; LY < Size; ++LY)
	{
		for (int32 LX = 0; LX < Size; ++LX)
		{
			if (IsCellBlocked(World, FIntPoint(Origin.X + LX - Radius, Origin.Y + LY - Radius), Z))
			{
				Blocked[LY * Size + LX] = true;
				++BlockedCount;
			}
		}
	}

	const int32 Start = ToIndex(Origin);
	const bool bHeroBlocked = Blocked[Start];
	// 영웅 칸이 막혀 보이면 맵의 큰 물체(하늘 구체 등)에 충돌이 있는 것: 잘못된 길로 막히지 않게 끔
	bHasObstacles = BlockedCount > 0 && !bHeroBlocked;

	if (BlockedCount != LastBlockedCount)
	{
		LastBlockedCount = BlockedCount;
		// MLVerify(M9-3d): 다음 마일스톤에서 제거
		UE_LOG(LogTemp, Display, TEXT("[MLVerify] Flow field: blocked=%d/%d heroBlocked=%d active=%d cache=%d ms=%.2f"),
			BlockedCount, Size * Size, bHeroBlocked ? 1 : 0, bHasObstacles ? 1 : 0, BlockedCache.Num(), (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
	}
	if (!bHasObstacles) return;

	// 영웅에서부터 너비 우선 탐색. 대각선은 양옆이 모두 비었을 때만 (모서리를 뚫지 않음)
	TArray<int32> Queue;
	Queue.Reserve(Size * Size);
	Distance[Start] = 0;
	Queue.Add(Start);
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const int32 Index = Queue[Head];
		const int32 LX = Index % Size;
		const int32 LY = Index / Size;
		for (const FIntPoint& Step : Neighbors)
		{
			const int32 NX = LX + Step.X;
			const int32 NY = LY + Step.Y;
			if (NX < 0 || NX >= Size || NY < 0 || NY >= Size) continue;
			const int32 Next = NY * Size + NX;
			if (Blocked[Next] || Distance[Next] >= 0) continue;
			if (Step.X != 0 && Step.Y != 0 && (Blocked[LY * Size + NX] || Blocked[NY * Size + LX])) continue;

			Distance[Next] = Distance[Index] + 1;
			Queue.Add(Next);
		}
	}
}

FVector FMLFlowField::GetMoveDir(const FVector& From, const FVector& HeroLoc) const
{
	const FVector Direct = (HeroLoc - From).GetSafeNormal2D();
	if (!bHasObstacles) return Direct;

	const FIntPoint Cell = ToCell(From);
	const int32 Index = ToIndex(Cell);
	const int32 MyDistance = Index != INDEX_NONE ? Distance[Index] : -1;
	if (MyDistance <= DirectCells) return Direct; // 창 밖, 갇힌 곳, 영웅 근처

	// 영웅에 더 가까운 이웃 칸 중 가장 가까운 칸으로
	int32 Best = MyDistance;
	FIntPoint BestCell = Cell;
	for (const FIntPoint& Step : Neighbors)
	{
		const FIntPoint Next = Cell + Step;
		const int32 NextIndex = ToIndex(Next);
		if (NextIndex == INDEX_NONE || Distance[NextIndex] < 0 || Distance[NextIndex] >= Best) continue;
		if (Step.X != 0 && Step.Y != 0)
		{
			const int32 SideA = ToIndex(FIntPoint(Next.X, Cell.Y));
			const int32 SideB = ToIndex(FIntPoint(Cell.X, Next.Y));
			if (SideA == INDEX_NONE || SideB == INDEX_NONE || Distance[SideA] < 0 || Distance[SideB] < 0) continue;
		}
		Best = Distance[NextIndex];
		BestCell = Next;
	}
	if (BestCell == Cell) return Direct;

	const FVector Target((BestCell.X + 0.5f) * CellSize, (BestCell.Y + 0.5f) * CellSize, From.Z);
	return (Target - From).GetSafeNormal2D();
}

FVector FMLFlowField::Slide(UWorld* World, const FVector& From, const FVector& To)
{
	if (!bHasObstacles || !IsBlocked(World, To)) return To;
	if (IsBlocked(World, From)) return To; // 이미 막힌 칸 안(스폰·밀어내기): 빠져나가게 둠

	const FVector OnlyX(To.X, From.Y, To.Z);
	if (!IsBlocked(World, OnlyX)) return OnlyX;
	const FVector OnlyY(From.X, To.Y, To.Z);
	if (!IsBlocked(World, OnlyY)) return OnlyY;
	return From;
}

FVector FMLFlowField::FindOpenLocation(UWorld* World, const FVector& Desired)
{
	if (!IsBlocked(World, Desired)) return Desired;

	for (int32 Ring = 1; Ring <= OpenSearchRings; ++Ring)
	{
		for (const FIntPoint& Step : Neighbors)
		{
			const FVector Candidate = Desired + FVector(Step.X, Step.Y, 0.f) * (Ring * CellSize);
			if (!IsBlocked(World, Candidate)) return Candidate;
		}
	}
	return Desired; // 주변이 모두 막힘: 원래 자리 (밀려 나오며 빠져나감)
}
