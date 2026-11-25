// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Combat/MLWeaponTypes.h"
#include "Enemy/MLEnemyActor.h"
#include "MLGhostActor.generated.h"

class UBlendSpace;
class USkeletalMeshComponent;

// design §6.2.3 망령. 보스처럼 UMLEnemySubsystem에 들어 있고(모든 기술이 평소처럼 맞힘),
// 이 액터는 옛 용병의 무기 방식으로 예고 공격 타이머만 돌림.
UCLASS()
class PROJECTA_API AMLGhostActor : public AMLEnemyActor
{
	GENERATED_BODY()

public:
	AMLGhostActor();

	// InDamageScale: 옛 용병 등급 보정
	void StartGhost(int32 InEnemyIndex, EMLAttackType InWeaponType, float InDamageScale);
	void StopGhost();
	int32 GetEnemyIndex() const { return EnemyIndex; }

protected:
	virtual void BeginPlay() override;

	// 사람 형태 (부모의 VisualMesh는 비워 둠)
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	// 정지·이동 블렌드 스페이스 (X = 방향, Y = 속도). Character가 아니라서 ABP 대신 직접 재생
	UPROPERTY(EditDefaultsOnly, Category = "Ghost")
	TObjectPtr<UBlendSpace> MoveBlendSpace;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost")
	float AttackDamage = 15.f;

	// 공격이 끝나고 다음 예고까지
	UPROPERTY(EditDefaultsOnly, Category = "Ghost", meta = (ClampMin = "0.2"))
	float AttackGap = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost")
	TSubclassOf<AMLSkillVisualActor> CircleTelegraphClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost")
	TSubclassOf<AMLSkillVisualActor> LineTelegraphClass;

	// 검 (단검 포함): 자기 주변 원
	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Sword")
	float SwordSeconds = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Sword")
	float SwordRadius = 300.f;

	// 활: 영웅 방향 직선
	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Bow")
	float BowSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Bow")
	float BowLength = 1400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Bow")
	float BowWidth = 160.f;

	// 지팡이: 예고를 시작할 때 영웅이 있던 자리의 원
	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Staff")
	float StaffSeconds = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Staff")
	float StaffRange = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|Staff")
	float StaffRadius = 250.f;

private:
	void BeginAttack();
	void ExecuteAttack();
	bool IsHit(const FVector& HeroLoc) const;
	void SetMoving(bool bMoving);

	FTimerHandle AttackTimerHandle;
	EMLAttackType WeaponType = EMLAttackType::Sword;
	float DamageScale = 1.f;
	FVector AttackOrigin = FVector::ZeroVector; // 검·지팡이 = 원의 중심, 활 = 쏘는 자리
	FVector AttackDir = FVector::ForwardVector; // 활
	int32 EnemyIndex = INDEX_NONE;

	static constexpr float RetrySeconds = 0.2f; // 사거리 밖: 다가가며 다시 확인
};
