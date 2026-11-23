// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLConfirmWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"

void UMLConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CancelButton->OnClicked.AddDynamic(this, &UMLConfirmWidget::HandleCancel);
}

void UMLConfirmWidget::Open(const FText& Title, const FText& Body, const TArray<FText>& Choices)
{
	TitleText->SetText(Title);
	BodyText->SetText(Body);
	if (UMLListEntryWidget::SyncPool(this, ChoiceClass, ChoiceList, ChoiceEntries, Choices.Num()))
	{
		for (int32 i = 0; i < Choices.Num(); ++i)
		{
			ChoiceEntries[i]->Setup(i, Choices[i], true);
			if (!ChoiceEntries[i]->OnEntryClicked.IsBound())
			{
				ChoiceEntries[i]->OnEntryClicked.BindUObject(this, &UMLConfirmWidget::HandleChoice);
			}
		}
	}
	SetVisibility(ESlateVisibility::Visible);
}

void UMLConfirmWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMLConfirmWidget::HandleCancel()
{
	Close();
}

void UMLConfirmWidget::HandleChoice(int32 Index)
{
	OnChoice.ExecuteIfBound(Index);
}
