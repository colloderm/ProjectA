// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLSlotIconWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

// 원정 HUD의 기술/패시브 슬롯 하나.
UCLASS(Abstract)
class PROJECTA_API UMLSlotIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// InIcon이 null = 색 사각형 + InName의 첫 글자 (아이콘 아트 전까지 임시)
	void SetFilled(UTexture2D* InIcon, const FText& InName, int32 InLevel, const FLinearColor& InFallbackColor);
	void SetEmpty(const FLinearColor& InEmptyColor);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LetterText;
};
