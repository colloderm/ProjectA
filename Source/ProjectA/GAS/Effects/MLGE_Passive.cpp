// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGE_Passive.h"

#include "AbilitySystemComponent.h"
#include "GAS/MLAttributeSet.h"

namespace
{
	constexpr int32 PassiveStatCount = static_cast<int32>(EMLPassiveStat::Count);

	// FName 숫자 접미사로 문자열 조립과 CDO 초기화 순서 문제를 피함.
	FName GetStatDataName(EMLPassiveStat Stat)
	{
		return FName(TEXT("Passive"), static_cast<int32>(Stat) + 1);
	}

	void GetStatModifier(EMLPassiveStat Stat, FGameplayAttribute& OutAttribute, EGameplayModOp::Type& OutOp)
	{
		OutOp = EGameplayModOp::Additive;
		switch (Stat)
		{
		case EMLPassiveStat::Attack:        OutAttribute = UMLAttributeSet::GetAttackAttribute();       OutOp = EGameplayModOp::MultiplyAdditive; break;
		case EMLPassiveStat::AttackSpeed:   OutAttribute = UMLAttributeSet::GetAttackSpeedAttribute();  break;
		case EMLPassiveStat::MaxHealth:     OutAttribute = UMLAttributeSet::GetMaxHealthAttribute();    OutOp = EGameplayModOp::MultiplyAdditive; break;
		case EMLPassiveStat::Defense:       OutAttribute = UMLAttributeSet::GetDefenseAttribute();      break;
		case EMLPassiveStat::CritChance:    OutAttribute = UMLAttributeSet::GetCritChanceAttribute();   break;
		case EMLPassiveStat::SkillCooldown: OutAttribute = UMLAttributeSet::GetSkillCooldownReductionAttribute(); break;
		case EMLPassiveStat::PickupRadius:  OutAttribute = UMLAttributeSet::GetPickupRadiusAttribute(); OutOp = EGameplayModOp::MultiplyAdditive; break;
		case EMLPassiveStat::XPGain:        OutAttribute = UMLAttributeSet::GetXPGainAttribute();       break;
		case EMLPassiveStat::HealthRegen:   OutAttribute = UMLAttributeSet::GetHealthRegenAttribute();  break;
		default: break;
		}
	}
}

UMLGE_Passive::UMLGE_Passive()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	for (int32 i = 0; i < PassiveStatCount; ++i)
	{
		const EMLPassiveStat Stat = static_cast<EMLPassiveStat>(i);

		FGameplayModifierInfo StatMod;
		EGameplayModOp::Type Op;
		GetStatModifier(Stat, StatMod.Attribute, Op);
		StatMod.ModifierOp = Op;

		FSetByCallerFloat ByCaller;
		ByCaller.DataName = GetStatDataName(Stat);
		StatMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(ByCaller);
		Modifiers.Add(StatMod);
	}
}

FGameplayEffectSpecHandle UMLGE_Passive::MakeSpec(const UAbilitySystemComponent& ASC, EMLPassiveStat Stat, float Value)
{
	FGameplayEffectSpecHandle SpecHandle = ASC.MakeOutgoingSpec(StaticClass(), 1.f, ASC.MakeEffectContext());
	if (!SpecHandle.IsValid()) return SpecHandle;

	for (int32 i = 0; i < PassiveStatCount; ++i)
	{
		const EMLPassiveStat EachStat = static_cast<EMLPassiveStat>(i);
		FGameplayAttribute UnusedAttribute;
		EGameplayModOp::Type Op;
		GetStatModifier(EachStat, UnusedAttribute, Op);

		// MultiplyAdditive: 1.0 = 변화 없음, 1.1 = +10%. Additive: 0 = 변화 없음.
		const bool bMultiply = Op == EGameplayModOp::MultiplyAdditive;
		const float Neutral = bMultiply ? 1.f : 0.f;
		const float Magnitude = EachStat == Stat ? (bMultiply ? 1.f + Value : Value) : Neutral;
		SpecHandle.Data->SetSetByCallerMagnitude(GetStatDataName(EachStat), Magnitude);
	}
	return SpecHandle;
}
