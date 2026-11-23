// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLDamageRowWidget.h"

#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Run/MLRunTypes.h"

void UMLDamageRowWidget::SetLine(const FMLDamageLine& Line, float TotalDamage, float CombatSeconds)
{
	const float Share = TotalDamage > 0.f ? Line.Damage / TotalDamage : 0.f;
	const int32 Dps = CombatSeconds > 0.f ? FMath::RoundToInt(Line.Damage / CombatSeconds) : 0;

	NameText->SetText(Line.Name);
	ValueText->SetText(FText::Format(NSLOCTEXT("ML", "DamageRowValue", "{0} · {1}% · DPS {2}"),
		FText::AsNumber(FMath::RoundToInt(Line.Damage)), FText::AsNumber(FMath::RoundToInt(Share * 100.f)), FText::AsNumber(Dps)));
	ShareBar->SetPercent(Share);
}

void UMLDamageRowWidget::FillList(UUserWidget* Owner, UPanelWidget* List, TSubclassOf<UMLDamageRowWidget> RowClass,
	const TArray<FMLDamageLine>& Lines, float CombatSeconds)
{
	if (!Owner || !List || !RowClass) return;

	float Total = 0.f;
	for (const FMLDamageLine& Line : Lines) Total += Line.Damage;

	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		UMLDamageRowWidget* Row = Cast<UMLDamageRowWidget>(List->GetChildAt(i));
		if (!Row)
		{
			Row = CreateWidget<UMLDamageRowWidget>(Owner, RowClass);
			if (!Row) return;
			List->AddChild(Row);
		}
		Row->SetLine(Lines[i], Total, CombatSeconds);
		Row->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	for (int32 i = Lines.Num(); i < List->GetChildrenCount(); ++i)
	{
		List->GetChildAt(i)->SetVisibility(ESlateVisibility::Collapsed);
	}
}
