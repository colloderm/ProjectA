// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.


#include "MLCombat.h"

#include "AbilitySystemComponent.h"
#include "GAS/MLAttributeSet.h"

float MLCombat::RollHeroDamage(const UAbilitySystemComponent& ASC, float DamageMult, bool bForceCrit)
{
	const float AttackValue = ASC.GetNumericAttribute(UMLAttributeSet::GetAttackAttribute());
	const float CritValue = ASC.GetNumericAttribute(UMLAttributeSet::GetCritChanceAttribute());

	float Damage = AttackValue * DamageMult * FMath::FRandRange(0.9f, 1.1f);
	if (bForceCrit || FMath::FRand() * 100.f < CritValue)
	{
		Damage *= 1.6f;
	}
	return Damage;
}
