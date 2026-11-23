// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLSlotIconWidget.h"

#include "Components/Image.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UMLSlotIconWidget::SetFilled(UTexture2D* InIcon, const FText& InName, int32 InLevel, const FLinearColor& InFallbackColor)
{
	if (InIcon)
	{
		IconImage->SetBrushFromTexture(InIcon);
		IconImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		// 텍스처 없는 브러시는 아무것도 그리지 않으므로, 단색 브러시로 바꿔 색 사각형을 그림
		IconImage->SetBrush(FSlateColorBrush(FLinearColor::White));
		IconImage->SetColorAndOpacity(InFallbackColor);
	}
	LevelText->SetText(FText::AsNumber(InLevel));

	if (LetterText)
	{
		LetterText->SetText(InIcon ? FText::GetEmpty() : FText::FromString(InName.ToString().Left(1)));
	}
}

void UMLSlotIconWidget::SetEmpty(const FLinearColor& InEmptyColor)
{
	IconImage->SetBrush(FSlateColorBrush(FLinearColor::White));
	IconImage->SetColorAndOpacity(InEmptyColor);
	LevelText->SetText(FText::GetEmpty());

	if (LetterText)
	{
		LetterText->SetText(FText::GetEmpty());
	}
}
