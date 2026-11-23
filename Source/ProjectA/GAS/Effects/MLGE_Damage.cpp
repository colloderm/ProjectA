// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGE_Damage.h"

#include "MLAttributeSet.h"

const FName UMLGE_Damage::DamageName(TEXT("Damage"));

UMLGE_Damage::UMLGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat DamageByCaller;
	DamageByCaller.DataName = DamageName;

	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute = UMLAttributeSet::GetIncomingDamageAttribute();
	DamageMod.ModifierOp = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageByCaller);
	Modifiers.Add(DamageMod);
}