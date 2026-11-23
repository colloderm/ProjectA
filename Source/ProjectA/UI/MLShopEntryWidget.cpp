// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLShopEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Run/MLRunProgressComponent.h"

void UMLShopEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EntryButton->OnClicked.AddDynamic(this, &UMLShopEntryWidget::HandleClicked);
}

void UMLShopEntryWidget::Setup(UMLRunProgressComponent* InRunProgress, int32 InItemIndex)
{
	RunProgress = InRunProgress;
	ItemIndex = InItemIndex;
	if (!RunProgress) return;

	EntryText->SetText(RunProgress->GetShopItemText(ItemIndex));
	EntryButton->SetIsEnabled(RunProgress->IsShopItemEnabled(ItemIndex));
}

void UMLShopEntryWidget::HandleClicked()
{
	if (RunProgress) RunProgress->TradeShopItem(ItemIndex);
}
