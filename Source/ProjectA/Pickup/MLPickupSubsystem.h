// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MLPickupSubsystem.generated.h"

class AMLPickupActor;
class AMLHeroCharacter;

DECLARE_MULTICAST_DELEGATE(FMLOnVacuumDone);

USTRUCT()
struct FMLGemEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AMLPickupActor> Actor = nullptr;

	float Value = 0.f;
	bool bAttracted = false;
	bool bActive = false;
};

UCLASS()
class PROJECTA_API UMLPickupSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	void SpawnGem(TSubclassOf<AMLPickupActor> GemClass, const FVector& Location, float Value);
	// 방 종료: 모든 활성 보석이 영웅에게 빠르게 날아감. 개수를 반환; 마지막 보석이 도착하면 OnVacuumDone 발생.
	int32 AttractAll();

	FMLOnVacuumDone OnVacuumDone;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void Deactivate(FMLGemEntry& Entry);

	UPROPERTY()
	TArray<FMLGemEntry> Gems;

	TWeakObjectPtr<AMLHeroCharacter> CachedHero;
	int32 ActiveCount = 0;
	bool bVacuum = false;

	static constexpr int32 MaxGems = 400;
	static constexpr float CollectRadius = 50.f;
	static constexpr float MagnetSpeed = 1200.f;
	static constexpr float VacuumSpeed = 3000.f;
};
