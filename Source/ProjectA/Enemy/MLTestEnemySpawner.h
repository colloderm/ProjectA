// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLTestEnemySpawner.generated.h"

class AMLEnemyActor;

UCLASS()
class PROJECTA_API AMLTestEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AMLTestEnemySpawner();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnBatch();

	UPROPERTY(EditAnywhere, Category = "Spawner")
	TSubclassOf<AMLEnemyActor> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.05"))
	float SpawnInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "1"))
	int32 SpawnPerTick = 2;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "1", ClampMax = "200"))
	int32 MaxAlive = 200;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	float SpawnRadius = 2000.f;

	FTimerHandle SpawnTimerHandle;
};
