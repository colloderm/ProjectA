// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLPickupSubsystem.h"

#include "AbilitySystemComponent.h"
#include "GAS/MLAttributeSet.h"
#include "Hero/MLHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/MLPickupActor.h"
#include "Run/MLRunProgressComponent.h"

bool UMLPickupSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UMLPickupSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMLPickupSubsystem, STATGROUP_Tickables);
}

void UMLPickupSubsystem::SpawnGem(TSubclassOf<AMLPickupActor> GemClass, const FVector& Location, float Value)
{
	if (!GemClass || Value <= 0.f) return;

	FMLGemEntry* Slot = Gems.FindByPredicate([&GemClass](const FMLGemEntry& G)
	{
		return !G.bActive && G.Actor && G.Actor->GetClass() == GemClass;
	});

	if (!Slot)
	{
		if (Gems.Num() >= MaxGems && !GemClass.GetDefaultObject()->bIsItem) // 상자는 합쳐지지 않음 (Value가 드롭 단계)
		{
			// shortcut: 풀이 가득 참 -> 가장 가까운 활성 보석에 값을 합침; 보이는 보석 수 < 실제 경험치
			FMLGemEntry* Nearest = nullptr;
			float BestDistSq = TNumericLimits<float>::Max();
			for (FMLGemEntry& Gem : Gems)
			{
				if (!Gem.bActive || !Gem.Actor || Gem.Actor->GetClass() != GemClass) continue;
				const float DistSq = FVector::DistSquared2D(Location, Gem.Actor->GetActorLocation());
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					Nearest = &Gem;
				}
			}
			if (Nearest) Nearest->Value += Value;
			return;
		}

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMLPickupActor* NewGem = GetWorld()->SpawnActor<AMLPickupActor>(GemClass, Location, FRotator::ZeroRotator, SpawnInfo);
		if (!NewGem) return;

		Slot = &Gems.AddDefaulted_GetRef();
		Slot->Actor = NewGem;
	}

	Slot->Value = Value;
	Slot->bAttracted = false;
	Slot->bActive = true;
	Slot->Actor->SetActorLocation(Location);
	Slot->Actor->SetActorHiddenInGame(false);
	++ActiveCount;
}

void UMLPickupSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ActiveCount == 0) return;

	AMLHeroCharacter* Hero = CachedHero.Get();
	if (!Hero)
	{
		Hero = Cast<AMLHeroCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		CachedHero = Hero;
	}
	if (!Hero || Hero->IsDead()) return;

	UMLRunProgressComponent* RunProgress = Hero->GetRunProgress();
	const UAbilitySystemComponent* ASC = Hero->GetAbilitySystemComponent();
	if (!RunProgress || !ASC) return;

	const FVector HeroLoc = Hero->GetActorLocation();
	const float MagnetRadiusSq = FMath::Square(ASC->GetNumericAttribute(UMLAttributeSet::GetPickupRadiusAttribute()));
	const float CollectRadiusSq = FMath::Square(CollectRadius);
	const float Speed = bVacuum ? VacuumSpeed : MagnetSpeed;

	for (FMLGemEntry& Entry : Gems)
	{
		if (!Entry.bActive || !Entry.Actor) continue;

		FVector Loc = Entry.Actor->GetActorLocation();
		const float DistSq = FVector::DistSquared2D(Loc, HeroLoc);

		if (!Entry.bAttracted && DistSq <= MagnetRadiusSq)
		{
			Entry.bAttracted = true; // 한 번 끌려오면 영웅까지 계속 날아감
		}
		if (!Entry.bAttracted) continue;

		if (DistSq <= CollectRadiusSq)
		{
			const float GemValue = Entry.Value;
			const bool bGold = Entry.Actor->bIsGold;
			const bool bItem = Entry.Actor->bIsItem;
			Deactivate(Entry);
			if (bItem)
			{
				RunProgress->AddChest(FMath::RoundToInt(GemValue));
			}
			else if (bGold)
			{
				RunProgress->AddGold(FMath::RoundToInt(GemValue));
			}
			else
			{
				RunProgress->AddXP(GemValue); // 게임을 일시정지할 수 있음; 남은 보석은 이번 프레임에 마저 처리
			}
			continue;
		}

		const float Dist = FMath::Sqrt(DistSq);
		FVector ToHero = HeroLoc - Loc;
		ToHero.Z = 0.f;
		Loc += (ToHero / Dist) * FMath::Min(Speed * DeltaTime, Dist);
		Entry.Actor->SetActorLocation(Loc);
	}

	if (bVacuum && ActiveCount == 0)
	{
		bVacuum = false;
		OnVacuumDone.Broadcast();
	}
}

void UMLPickupSubsystem::Deactivate(FMLGemEntry& Entry)
{
	Entry.bActive = false;
	Entry.Actor->SetActorHiddenInGame(true);
	--ActiveCount;
}

int32 UMLPickupSubsystem::AttractAll()
{
	int32 Attracted = 0;
	for (FMLGemEntry& Entry : Gems)
	{
		if (!Entry.bActive || !Entry.Actor) continue;
		Entry.bAttracted = true;
		++Attracted;
	}
	bVacuum = Attracted > 0; // Tick이 VacuumSpeed로 옮기고 도착하면 경험치 지급
	return Attracted;
}
