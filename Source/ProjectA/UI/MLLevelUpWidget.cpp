// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLLevelUpWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Run/MLRunProgressComponent.h"

void UMLLevelUpWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ChoiceButton0->OnClicked.AddDynamic(this, &UMLLevelUpWidget::HandleChoice0);
	ChoiceButton1->OnClicked.AddDynamic(this, &UMLLevelUpWidget::HandleChoice1);
	ChoiceButton2->OnClicked.AddDynamic(this, &UMLLevelUpWidget::HandleChoice2);
	RerollButton->OnClicked.AddDynamic(this, &UMLLevelUpWidget::HandleReroll);
}

void UMLLevelUpWidget::Setup(UMLRunProgressComponent* InRunProgress)
{
	RunProgress = InRunProgress;
}

void UMLLevelUpWidget::Refresh()
{
	if (!RunProgress) return;

	UButton* Buttons[] = { ChoiceButton0.Get(), ChoiceButton1.Get(), ChoiceButton2.Get() };
	URichTextBlock* Texts[] = { ChoiceText0.Get(), ChoiceText1.Get(), ChoiceText2.Get() };

	for (int32 i = 0; i < UE_ARRAY_COUNT(Buttons); ++i)
	{
		const bool bUsed = i < RunProgress->GetChoiceCount();
		Buttons[i]->SetVisibility(bUsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Buttons[i]->SetIsEnabled(RunProgress->IsChoiceEnabled(i));
		Texts[i]->SetText(RunProgress->GetChoiceText(i));

		const int32 Tier = RunProgress->GetChoiceTier(i);
		Buttons[i]->SetBackgroundColor(Tier >= 3 ? Tier3CardTint : (Tier == 2 ? Tier2CardTint : DefaultCardTint));
	}

	const bool bRest = RunProgress->IsRestOpen();
	RerollButton->SetVisibility(bRest ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	RerollButton->SetIsEnabled(RunProgress->CanReroll());
	if (TitleText)
	{
		TitleText->SetText(bRest ? NSLOCTEXT("ML", "RestTitle", "모닥불") : NSLOCTEXT("ML", "LevelUpTitle", "레벨 업!"));
	}
}

void UMLLevelUpWidget::FocusFirstChoice(APlayerController* PC)
{
	if (PC)
	{
		ChoiceButton0->SetUserFocus(PC); // 게임패드 탐색은 여기서 시작
	}
}

void UMLLevelUpWidget::HandleChoice0() { if (RunProgress) RunProgress->SelectChoice(0); }
void UMLLevelUpWidget::HandleChoice1() { if (RunProgress) RunProgress->SelectChoice(1); }
void UMLLevelUpWidget::HandleChoice2() { if (RunProgress) RunProgress->SelectChoice(2); }
void UMLLevelUpWidget::HandleReroll()  { if (RunProgress) RunProgress->Reroll(); }
