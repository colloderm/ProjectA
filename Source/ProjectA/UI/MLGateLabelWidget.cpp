// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGateLabelWidget.h"

#include "Components/TextBlock.h"

void UMLGateLabelWidget::SetLabel(const FText& InText)
{
	LabelText->SetText(InText);
}
