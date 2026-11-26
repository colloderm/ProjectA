// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLProjectileActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class PROJECTA_API AMLProjectileActor : public AActor
{
	GENERATED_BODY()

public:
	AMLProjectileActor();

	// design §6.2.3 망령의 탄 표시. nullptr = 원래 모습
	void SetOverlay(UMaterialInterface* Overlay);

private:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
};
