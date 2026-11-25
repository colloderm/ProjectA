// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLTitleWidget.h"

#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "UI/MLGuildUIText.h"

void UMLTitleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CloseButton->OnClicked.AddDynamic(this, &UMLTitleWidget::HandleClose);
}

void UMLTitleWidget::Open()
{
	bLockedView = false;
	SetVisibility(ESlateVisibility::Visible);
	Refresh();
}

void UMLTitleWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UMLTitleWidget::Refresh()
{
	if (!IsVisible()) return;
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save || !Save->bHasMerc)
	{
		Close(); // 용병이 없으면 칭호를 바꿀 대상이 없음
		return;
	}

	HeaderText->SetText(FText::Format(NSLOCTEXT("ML", "TitleHeader", "<Name>{0}</>의 칭호: {1}"),
		FText::FromString(Save->Merc.Name), MLGuildUI::TitleName(*Guild, Save->Merc.Title)));

	TArray<FName> Unlocked;
	TArray<FName> Locked;
	for (const FName& Id : Guild->GetTitleIds())
	{
		(Guild->IsTitleUnlocked(Id) ? Unlocked : Locked).Add(Id);
	}

	// [획득 N] [미획득 N]: 창고의 [보관함] [상점]과 같은 탭 항목
	if (UMLListEntryWidget::SyncPool(this, ViewTabClass, ViewTabList, ViewTabs, 2))
	{
		const FText Names[] = {
			FText::Format(NSLOCTEXT("ML", "TitleViewUnlocked", "획득 {0}"), FText::AsNumber(Unlocked.Num())),
			FText::Format(NSLOCTEXT("ML", "TitleViewLocked", "미획득 {0}"), FText::AsNumber(Locked.Num())) };
		for (int32 t = 0; t < 2; ++t)
		{
			const bool bSelected = (t == 1) == bLockedView;
			ViewTabs[t]->Setup(t, bSelected ? FText::Format(NSLOCTEXT("ML", "TitleViewSelected", "<Selected>{0}</>"), Names[t]) : Names[t], true);
			ViewTabs[t]->SetSelected(bSelected);
			if (!ViewTabs[t]->OnEntryClicked.IsBound()) ViewTabs[t]->OnEntryClicked.BindUObject(this, &UMLTitleWidget::HandleViewTab);
		}
	}

	ShownIds = bLockedView ? Locked : Unlocked;
	if (!bLockedView) ShownIds.Insert(NAME_None, 0);
	if (!UMLListEntryWidget::SyncPool(this, EntryClass, TitleList, Entries, ShownIds.Num())) return;
	for (int32 i = 0; i < ShownIds.Num(); ++i)
	{
		const FName Id = ShownIds[i];
		const FText Label = bLockedView
			? MLGuildUI::TitleProgressLine(*Guild, Id)
			: Id.IsNone() ? NSLOCTEXT("ML", "TitleNoneEntry", "칭호 없음") : MLGuildUI::TitleLine(*Guild, Id);
		Entries[i]->Setup(i, Label, !bLockedView);
		Entries[i]->SetSelected(!bLockedView && Id == Save->Merc.Title); // 장착 중 = 선택 색
		if (!Entries[i]->OnEntryClicked.IsBound()) Entries[i]->OnEntryClicked.BindUObject(this, &UMLTitleWidget::HandleEntry);
	}
}

void UMLTitleWidget::HandleClose()
{
	Close();
}

void UMLTitleWidget::HandleViewTab(int32 TabIndex)
{
	bLockedView = TabIndex == 1;
	Refresh();
}

void UMLTitleWidget::HandleEntry(int32 EntryIndex)
{
	UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	if (Guild && !bLockedView && ShownIds.IsValidIndex(EntryIndex))
	{
		Guild->EquipTitle(ShownIds[EntryIndex]); // Save() → OnGuildChanged → 허브 Refresh → 이 창 Refresh
	}
}
