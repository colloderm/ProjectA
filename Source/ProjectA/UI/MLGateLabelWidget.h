// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLGateLabelWidget.generated.h"

class UTextBlock;

// 포털 / 탈출 문 위의 방 종류 라벨, AMLRoomGateActor::LabelWidget이 표시 (스크린 공간).
UCLASS(Abstract)
class PROJECTA_API UMLGateLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLabel(const FText& InText);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;
};
