// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLPickupActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PROJECTA_API AMLPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AMLPickupActor();
	
	// true = 금화: 주우면 경험치 대신 원정 골드 추가 (Value = 골드)
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	bool bIsGold = false;
	
	// true = 아이템 상자: Value = 드롭 단계 1..4 (design §6.3); 아이템은 주울 때 추첨
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	bool bIsItem = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USceneComponent> PickupRoot;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;
};
