// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLShopWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Run/MLRunGameMode.h"
#include "Run/MLRunProgressComponent.h"
#include "UI/MLShopEntryWidget.h"

void UMLShopWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	ExitButton->OnClicked.AddDynamic(this, &UMLShopWidget::HandleExit);
	BuyTabButton->OnClicked.AddDynamic(this, &UMLShopWidget::HandleBuyTab);
	SellTabButton->OnClicked.AddDynamic(this, &UMLShopWidget::HandleSellTab);
}

void UMLShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bSellTab = false; // 방문할 때마다 구매 탭으로 열림; OpenShop이 AddToViewport 직후 Refresh를 호출
	ExitButton->SetUserFocus(GetOwningPlayer()); // 게임패드는 "나가기"에서 시작
}

void UMLShopWidget::Setup(UMLRunProgressComponent* InRunProgress)
{
	RunProgress = InRunProgress;
}

void UMLShopWidget::Refresh()
{
	if (!RunProgress) return;

	const AMLRunGameMode* RunMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMLRunGameMode>() : nullptr;
	GoldText->SetText(FText::Format(NSLOCTEXT("ML", "ShopGold", "골드 {0} · 탐욕 {1}/{2}"),
		FText::AsNumber(RunProgress->GetRunGold()),
		FText::AsNumber(RunMode ? RunMode->GetGreedLevel() : 0),
		FText::AsNumber(RunMode ? RunMode->GetMaxGreedLevel() : 0)));

	BuyTabButton->SetIsEnabled(bSellTab);
	SellTabButton->SetIsEnabled(!bSellTab);

	// 행은 전체 상점 인덱스를 유지하므로 구매 / 판매는 여전히 인덱스로 RunProgress를 거침.
	TArray<int32> TabItems;
	for (int32 i = 0; i < RunProgress->GetShopItemCount(); ++i)
	{
		if (RunProgress->IsShopSellItem(i) == bSellTab) TabItems.Add(i);
	}

	const int32 Count = TabItems.Num();
	while (RowClass && Rows.Num() < Count)
	{
		UMLShopEntryWidget* Row = CreateWidget<UMLShopEntryWidget>(this, RowClass);
		if (!Row) break;
		if (UVerticalBoxSlot* RowSlot = ItemList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, RowSpacing));
		}
		Rows.Add(Row);
	}

	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const bool bUsed = i < Count;
		Rows[i]->SetVisibility(bUsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bUsed) Rows[i]->Setup(RunProgress, TabItems[i]);
	}
}

void UMLShopWidget::SetTab(bool bSell)
{
	bSellTab = bSell;
	Refresh();
	(bSell ? BuyTabButton : SellTabButton)->SetUserFocus(GetOwningPlayer()); // 누른 탭은 이제 비활성; 게임패드 포커스를 유지
}

void UMLShopWidget::HandleBuyTab()
{
	SetTab(false);
}

void UMLShopWidget::HandleSellTab()
{
	SetTab(true);
}

void UMLShopWidget::HandleExit()
{
	if (RunProgress) RunProgress->CloseShop();
}
