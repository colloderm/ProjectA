// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLContractWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Guild/MLGuildSubsystem.h"
#include "Guild/MLSaveGame.h"
#include "UI/MLGuildUIText.h"
#include "UI/MLListEntryWidget.h"

void UMLContractWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RerollButton->OnClicked.AddDynamic(this, &UMLContractWidget::HandleReroll);
}

void UMLContractWidget::Refresh()
{
	const UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this);
	const UMLSaveGame* Save = Guild ? Guild->GetSaveData() : nullptr;
	if (!Save) return;

	// design §8.3: 모든 등급에 탭이 있음; 잠긴 탭은 필요한 명성을 표시.
	// 선택된 탭은 활성 유지 (다시 누르면 갱신만); SetSelected가 앰버색으로 칠하고, <Selected>가 그 위 글자를 어둡게 유지.
	constexpr int32 GradeCount = static_cast<int32>(EMLGrade::Count);
	if (UMLListEntryWidget::SyncPool(this, TabClass, GradeTabList, Tabs, GradeCount))
	{
		for (int32 g = 0; g < GradeCount; ++g)
		{
			const EMLGrade Grade = static_cast<EMLGrade>(g);
			const FMLGradeRow* Row = Guild->FindGrade(Grade);
			const bool bUnlocked = Row && Save->Renown >= Row->UnlockRenown;
			const bool bSelected = Grade == SelectedGrade;
			const FText Label = bSelected
				? FText::Format(NSLOCTEXT("ML", "ContractTabSelected", "<Selected>{0}</>"), MLGuildUI::GradeText(Grade))
				: bUnlocked
					? MLGuildUI::GradeText(Grade)
					: FText::Format(NSLOCTEXT("ML", "ContractTabLocked", "{0} (명성 {1})"),
						MLGuildUI::GradeText(Grade), FText::AsNumber(Row ? Row->UnlockRenown : 0));
			Tabs[g]->Setup(g, Label, bUnlocked);
			Tabs[g]->SetSelected(bSelected);
			if (!Tabs[g]->OnEntryClicked.IsBound()) Tabs[g]->OnEntryClicked.BindUObject(this, &UMLContractWidget::HandleGradeTab);
		}
	}

	// design §8.1: 선택한 등급의 후보 카드 (Index = 해당 등급의 n번째 후보, Contract()와 같음)
	const FMLGradeRow* SelectedRow = Guild->FindGrade(SelectedGrade);
	const int32 Price = SelectedRow ? SelectedRow->ContractGold : 0;
	TArray<const FMLMercData*> Shown;
	for (const FMLMercData& Candidate : Save->Candidates)
	{
		if (Candidate.Grade == SelectedGrade) Shown.Add(&Candidate);
	}
	if (UMLListEntryWidget::SyncPool(this, CardClass, CandidateList, Cards, Shown.Num()))
	{
		for (int32 i = 0; i < Shown.Num(); ++i)
		{
			const FText CardText = FText::Format(NSLOCTEXT("ML", "ContractCard", "{0}\n\n<Dim>계약금</> <Gold>{1}G</>"),
				MLGuildUI::DescribeMerc(*Guild, *Shown[i]), FText::AsNumber(Price));
			Cards[i]->Setup(i, CardText, !Save->bHasMerc && Save->Gold >= Price);
			if (!Cards[i]->OnEntryClicked.IsBound()) Cards[i]->OnEntryClicked.BindUObject(this, &UMLContractWidget::HandleCard);
		}
	}

	InfoText->SetText(Save->bHasMerc
		? NSLOCTEXT("ML", "ContractOwned", "이미 용병을 보유 중입니다. 은퇴하거나 판매한 뒤 계약할 수 있습니다.")
		: FText::Format(NSLOCTEXT("ML", "ContractInfo", "명성 {0} · 원정을 마칠 때마다 후보가 바뀝니다."), FText::AsNumber(Save->Renown)));

	const int32 RerollCost = Guild->GetRerollCost();
	RerollText->SetText(FText::Format(NSLOCTEXT("ML", "ContractReroll", "후보 교체 ({0}G)"), FText::AsNumber(RerollCost)));
	RerollButton->SetIsEnabled(Save->Gold >= RerollCost);
}

void UMLContractWidget::HandleGradeTab(int32 GradeIndex)
{
	SelectedGrade = static_cast<EMLGrade>(GradeIndex);
	Refresh();
}

void UMLContractWidget::HandleCard(int32 IndexInGrade)
{
	OnContractRequested.ExecuteIfBound(SelectedGrade, IndexInGrade);
}

void UMLContractWidget::HandleReroll()
{
	if (UMLGuildSubsystem* Guild = MLGuildUI::GetGuild(this))
	{
		Guild->RerollCandidates(); // Save() → OnGuildChanged → 허브 갱신
	}
}
