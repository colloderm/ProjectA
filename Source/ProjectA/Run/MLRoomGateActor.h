// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Run/MLRunTypes.h"
#include "MLRoomGateActor.generated.h"

class UStaticMeshComponent;
class UWidgetComponent;

// 다음 방으로 가는 포털, 또는 구역 끝 탈출 문. 표시 전용; 선택은 AMLRunGameMode가 처리.
UCLASS()
class PROJECTA_API AMLRoomGateActor : public AActor
{
	GENERATED_BODY()

public:
	AMLRoomGateActor();

	void InitGate(EMLRoomType InRoomType, bool bInExitDoor);
	void SetChannelProgress(float Ratio); // 0 = 대기, 1 = 선택됨

	EMLRoomType GetRoomType() const { return RoomType; }
	bool IsExitDoor() const { return bExitDoor; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<USceneComponent> GateRoot;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	UPROPERTY(VisibleAnywhere, Category = "Gate")
	TObjectPtr<UWidgetComponent> LabelWidget;

private:
	FText GetBaseLabel() const;
	void ApplyLabel(const FText& Text);

	EMLRoomType RoomType = EMLRoomType::Normal;
	bool bExitDoor = false;
};
