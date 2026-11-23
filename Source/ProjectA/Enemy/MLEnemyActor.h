// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLEnemyActor.generated.h"

class UStaticMeshComponent;
class AMLPickupActor;
class UMaterialInterface;

UCLASS()
class PROJECTA_API AMLEnemyActor : public AActor
{
	GENERATED_BODY()

public:
	AMLEnemyActor();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float MaxHealth = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float MoveSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	float ContactDamage = 5.f;
	
	// 큰 적: 접촉·기술 명중 거리에 더해짐 (아니면 중심 간 거리). 일반 적은 0.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0"))
	float BodyRadius = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	float XPValue = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	TSubclassOf<AMLPickupActor> XPGemClass;
	
	// design §6.3: 사망 시 GoldChance 판정 -> GoldValue짜리 금화 GoldDropCount개 드롭 (× 구역 GoldMult)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop", meta = (ClampMin = "0", ClampMax = "1"))
	float GoldChance = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop", meta = (ClampMin = "0"))
	float GoldValue = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop", meta = (ClampMin = "1"))
	int32 GoldDropCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Drop")
	TSubclassOf<AMLPickupActor> GoldCoinClass;

	// 황금 슬라임 (design §6.2.1): 영웅을 쫓지 않고 도망침
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	bool bFleesFromHero = false;
	
	// design §6.5 엘리트 외형: 액터 크기 + 오버레이 머티리얼 (비면 크기만)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Elite", meta = (ClampMin = "1"))
	float EliteScale = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Elite")
	TObjectPtr<UMaterialInterface> EliteOverlayMaterial;

	// 풀링된 액터는 스폰될 때마다 일반/엘리트로 전환됨.
	void SetEliteLook(bool bElite);

private:
	UPROPERTY(VisibleAnywhere, Category = "Enemy")
	TObjectPtr<USceneComponent> EnemyRoot;

	UPROPERTY(VisibleAnywhere, Category = "Enemy")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
};
