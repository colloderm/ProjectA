// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GAS/MLGameplayTags.h"
#include "Hero/MLHeroCharacter.h"

void UMLAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetDashCooldownReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 0.5f); // design §7.2: 최대 50%
	}
	else if (Attribute == GetDodgeChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 10.f); // design §7.2 상한
	}
	else if (Attribute == GetCritChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 75.f); // design §7.2 상한
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, -90.f); // 공격 간격이 유한하도록 유지
	}
	else if (Attribute == GetSkillCooldownReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 0.5f);
	}
	else if (Attribute == GetPickupRadiusAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetHealthRegenAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UMLAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute != GetIncomingDamageAttribute()) return;

	const float RawDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	UAbilitySystemComponent& TargetASC = Data.Target;
	if (RawDamage <= 0.f 
		|| TargetASC.HasMatchingGameplayTag(MLTags::State_Invulnerable.GetTag()) 
		|| TargetASC.HasMatchingGameplayTag(MLTags::State_Dead.GetTag())) return;
	
	// design §7.2: DODGE는 타격 전체를 무효화
	if (FMath::FRand() * 100.f < GetDodgeChance()) return;

	// design §7.2: 피해 x 100/(100+DEF)
	float FinalDamage = RawDamage * 100.f / (100.f + FMath::Max(0.f, GetDefense()));

	// 수호 방패 쉴드가 HP보다 먼저 피해를 흡수.
	const float Absorbed = FMath::Min(GetShield(), FinalDamage);
	if (Absorbed > 0.f)
	{
		SetShield(GetShield() - Absorbed);
		FinalDamage -= Absorbed;
	}
	if (FinalDamage <= 0.f) return;

	SetHealth(FMath::Clamp(GetHealth() - FinalDamage, 0.f, GetMaxHealth()));

	if (GetHealth() <= 0.f)
	{
		if (AMLHeroCharacter* Hero = Cast<AMLHeroCharacter>(TargetASC.GetAvatarActor()))
		{
			Hero->HandleDeath();
		}
	}
}

void UMLAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute != GetMaxHealthAttribute()) return;

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC) return;

	// 최대 HP가 오르면 같은 양을 회복; 내려가면 현재 HP를 잘라냄.
	if (NewValue > OldValue)
	{
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Additive, NewValue - OldValue);
	}
	else if (GetHealth() > NewValue)
	{
		ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
	}
}
