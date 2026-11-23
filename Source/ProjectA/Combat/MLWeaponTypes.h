// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MLWeaponTypes.generated.h"

class AMLProjectileActor;

UENUM(BlueprintType)
enum class EMLAttackType : uint8
{
	Dagger,
	Sword,
	Bow,
	Staff
};

// DT_Weapons 행. 아이템 필드(RequiredGrade, tier...)는 M7에서 추가.
USTRUCT(BlueprintType)
struct FMLWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EMLAttackType AttackType = EMLAttackType::Dagger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float DamageMult = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.05"))
	float BaseInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Range = 500.f;

	// 지팡이 폭발 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AoERadius = 200.f;

	// 검 전체 호 각도(도)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float ArcAngle = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	TSubclassOf<AMLProjectileActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	float ProjectileSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	float ProjectileHitRadius = 50.f;
};