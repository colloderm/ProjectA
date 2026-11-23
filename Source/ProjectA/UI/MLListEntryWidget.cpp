// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLListEntryWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"

void UMLListEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EntryButton->OnClicked.AddDynamic(this, &UMLListEntryWidget::HandleClicked);
	UnselectedStyle = EntryButton->GetStyle();
}

void UMLListEntryWidget::Setup(int32 InEntryIndex, const FText& Text, bool bEnabled)
{
	EntryIndex = InEntryIndex;
	EntryText->SetText(Text);
	EntryButton->SetIsEnabled(bEnabled);
	SetVisibility(ESlateVisibility::Visible);
}

void UMLListEntryWidget::SetSelected(bool bSelected, const FLinearColor* BaseTint)
{
	FButtonStyle Style = UnselectedStyle;
	if (bSelected)
	{
		Style.Normal.TintColor = SelectedTint;
		Style.Hovered.TintColor = SelectedTint;
		Style.Pressed.TintColor = SelectedTint;
	}
	else if (BaseTint)
	{
		Style.Normal.TintColor = *BaseTint;
		Style.Hovered.TintColor = (*BaseTint * 1.8f).CopyWithNewOpacity(1.f); // 같은 색조, 호버 시 더 밝게
		Style.Pressed.TintColor = *BaseTint;
	}
	EntryButton->SetStyle(Style);
}

void UMLListEntryWidget::HandleClicked()
{
	OnEntryClicked.ExecuteIfBound(EntryIndex);
}

bool UMLListEntryWidget::SyncPool(UUserWidget* Owner, TSubclassOf<UMLListEntryWidget> EntryClass, UPanelWidget* Panel,
	TArray<TObjectPtr<UMLListEntryWidget>>& Pool, int32 Count)
{
	if (!Owner || !Panel || !EntryClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] UI: %s is missing an entry class or list panel"), *GetNameSafe(Owner));
		return false;
	}

	while (Pool.Num() < Count)
	{
		UMLListEntryWidget* Entry = CreateWidget<UMLListEntryWidget>(Owner, EntryClass);
		if (!Entry) return false;
		Panel->AddChild(Entry);
		Pool.Add(Entry);
	}
	for (int32 i = Count; i < Pool.Num(); ++i)
	{
		Pool[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
	return true;
}
