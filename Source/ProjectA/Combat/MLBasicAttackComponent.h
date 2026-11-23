// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "MLBasicAttackComponent.generated.h"

struct FMLWeaponRow;

UCLASS(ClassGroup=(ML), meta=(BlueprintSpawnableComponent))
class PROJECTA_API UMLBasicAttackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMLBasicAttackComponent();

protected:
	virtual void BeginPlay() override;

public:
	void ScheduleNextAttack();
	void PerformAttack();
	const FMLWeaponRow* GetWeaponRow() const;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	FDataTableRowHandle WeaponRowHandle;
	
	// design §7.4 주무기 일치 보너스, 빙의 시 영웅이 설정
	float DamageScale = 1.f;

	FTimerHandle AttackTimerHandle;
};
