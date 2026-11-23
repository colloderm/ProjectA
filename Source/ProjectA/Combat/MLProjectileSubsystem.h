// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MLProjectileSubsystem.generated.h"

class AMLProjectileActor;

// 탄 하나에 필요한 모든 값. 기본값 = 일반 직선 탄 (활).
struct FMLProjectileLaunch
{
	TSubclassOf<AMLProjectileActor> ProjectileClass;
	FVector Start = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.f;
	float HitRadius = 50.f;
	float MaxRange = 1000.f;
	float SlowFactor = 1.f;     // 명중 시 적 속도 배율, 1 = 둔화 없음
	float SlowDuration = 0.f;
	float HomingTurnRate = 0.f; // 초당 각도, 0 = 직선
	FName Source;               // 피해 리포트 키 (design §6 HUD, M9-0)
};

USTRUCT()
struct FMLProjectileEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AMLProjectileActor> Actor = nullptr;

	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.f;
	float HitRadius = 0.f;
	float RangeLeft = 0.f;
	float SlowFactor = 1.f;
	float SlowDuration = 0.f;
	float HomingTurnRate = 0.f;
	FName Source;
	bool bActive = false;
};

UCLASS()
class PROJECTA_API UMLProjectileSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// 풀 한도에 도달하면 false 반환.
	bool Launch(const FMLProjectileLaunch& LaunchInfo);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void Deactivate(FMLProjectileEntry& Entry);

	UPROPERTY()
	TArray<FMLProjectileEntry> Projectiles;

	int32 ActiveCount = 0;

	static constexpr int32 MaxProjectiles = 300;
	static constexpr float HomingSeekRange = 800.f;
};
