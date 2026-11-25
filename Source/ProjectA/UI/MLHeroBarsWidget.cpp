// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLHeroBarsWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GAS/MLAttributeSet.h"

void UMLHeroBarsWidget::InitBars(UAbilitySystemComponent* ASC)
{
	BoundASC = ASC;

	// AddUObject는 약한 참조: 위젯이 먼저 수거돼도 안전.
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetHealthAttribute()).AddUObject(this, &UMLHeroBarsWidget::HandleAttributeChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UMLHeroBarsWidget::HandleAttributeChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UMLAttributeSet::GetShieldAttribute()).AddUObject(this, &UMLHeroBarsWidget::HandleAttributeChanged);
	}

	RefreshBars();
}

void UMLHeroBarsWidget::SetTitle(const FText& TitleName, EMLTitleRarity Rarity)
{
	if (!TitleText) return;

	TitleText->SetText(TitleName);
	const int32 ColorIndex = static_cast<int32>(Rarity);
	if (TitleColors.IsValidIndex(ColorIndex))
	{
		TitleText->SetColorAndOpacity(FSlateColor(TitleColors[ColorIndex]));
	}
	TitleText->SetVisibility(TitleName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UMLHeroBarsWidget::HandleAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	// design §6.4: 쉴드는 수호 방패가 발동할 때만 오르므로, 오른 양 = 새로 받은 양 (100%)
	if (ChangeData.Attribute == UMLAttributeSet::GetShieldAttribute() && ChangeData.NewValue > ChangeData.OldValue)
	{
		ShieldPeak = ChangeData.NewValue;
	}
	RefreshBars();
}

void UMLHeroBarsWidget::RefreshBars()
{
	const UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	const float MaxHP = ASC->GetNumericAttribute(UMLAttributeSet::GetMaxHealthAttribute());
	if (MaxHP <= 0.f) return; // 어트리뷰트가 아직 설정되지 않음 (BeginPlay가 PossessedBy보다 먼저 실행될 수 있음)

	const float CurrentHP = ASC->GetNumericAttribute(UMLAttributeSet::GetHealthAttribute());
	const float ShieldValue = ASC->GetNumericAttribute(UMLAttributeSet::GetShieldAttribute());
	TargetHealth = FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f);
	TargetShield = ShieldPeak > 0.f ? FMath::Clamp(ShieldValue / ShieldPeak, 0.f, 1.f) : 0.f;

	if (ShownHealth < 0.f)
	{
		ShownHealth = TargetHealth;
		ShownShield = TargetShield;
	}
	SetBarTarget(ShownHealth, TargetHealth, HealthFillRate);
	SetBarTarget(ShownShield, TargetShield, ShieldFillRate);
	ApplyBars();

	if (ShieldBar)
	{
		// 쉴드가 깨질 때 HP 바가 튀지 않도록 Collapsed가 아닌 Hidden
		ShieldBar->SetVisibility(ShieldValue > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UMLHeroBarsWidget::SetBarTarget(float& Shown, float Target, float& FillRate) const
{
	// 내려갈 때 = 즉시. 오를 때 = FillSeconds 동안 채움; 채우는 중 피해를 받으면 Target이 낮아져서 줄어든 값에서 멈춤.
	if (Target <= Shown)
	{
		Shown = Target;
		FillRate = 0.f;
	}
	else
	{
		FillRate = (Target - Shown) / FillSeconds;
	}
}

void UMLHeroBarsWidget::ApplyBars()
{
	HealthBar->SetPercent(ShownHealth);
	if (ShieldBar) ShieldBar->SetPercent(ShownShield);
}

void UMLHeroBarsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 조기 종료: 채우는 중인 것이 없음 (첫 유효값 이전 포함)
	if (ShownHealth < 0.f || (ShownHealth >= TargetHealth && ShownShield >= TargetShield)) return;

	ShownHealth = FMath::Min(TargetHealth, ShownHealth + HealthFillRate * InDeltaTime);
	ShownShield = FMath::Min(TargetShield, ShownShield + ShieldFillRate * InDeltaTime);
	ApplyBars();
}
