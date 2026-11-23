// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLProjectileActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PROJECTA_API AMLProjectileActor : public AActor
{
	GENERATED_BODY()

public:
	AMLProjectileActor();

private:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
};
