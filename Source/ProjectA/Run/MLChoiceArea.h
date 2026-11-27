// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLChoiceArea.generated.h"

class UArrowComponent;

// design §6.2 선택 구역 (M9-3e): 방이 끝나면 영웅이 이 액터 위치로 순간이동해 포털을 고름.
// 맵에 하나만 둠. 영웅 캡슐 중심 높이(PlayerStart와 같은 Z)에 놓고, 바닥은 전투 구역과 같은 높이.
// 맵에 없으면 예전처럼 방이 끝난 자리 앞에 포털이 나옴.
UCLASS()
class PROJECTA_API AMLChoiceArea : public AActor
{
	GENERATED_BODY()

public:
	AMLChoiceArea();

private:
	UPROPERTY(VisibleAnywhere, Category = "ChoiceArea")
	TObjectPtr<USceneComponent> AreaRoot;

	// 에디터에서 위치 확인용 (게임에서는 숨김)
	UPROPERTY(VisibleAnywhere, Category = "ChoiceArea")
	TObjectPtr<UArrowComponent> Arrow;
};
