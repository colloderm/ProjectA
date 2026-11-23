// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MLAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UMLAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, MaxHealth)

	// 0..0.5 (M7부터 DODGE 스탯에서)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData DashCooldownReduction;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, DashCooldownReduction)
	
	// 퍼센트, 0..10 (design §7.2: DODGE × 0.2%) — 타격 무효화 확률
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData DodgeChance;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, DodgeChance)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, Defense)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, Attack)

	// SPD: 기본 공격 간격 = BaseInterval / (1 + AttackSpeed/100)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, AttackSpeed)

	// 퍼센트, 0..75
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, CritChance)
	
	// 보석 자석 반경 (uu), 기본 150
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData PickupRadius;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, PickupRadius)

	// 원정 경험치 퍼센트 보너스
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData XPGain;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, XPGain)

	// 0..0.5, M5의 자동 기술에서 사용
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData SkillCooldownReduction;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, SkillCooldownReduction)
	
	// 수호 방패 쉴드: Health보다 먼저 피해 흡수 (UMLSkill_GuardShield가 설정)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, Shield)
	
	// 초당 MaxHealth 대비 회복 퍼센트 (design §6.4 재생 패시브; AMLHeroCharacter가 0.5초마다 회복)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData HealthRegen;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, HealthRegen)

	// 메타 어트리뷰트: UMLGE_Damage가 쓰고 PostGameplayEffectExecute에서 소비, 항상 0으로 초기화
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UMLAttributeSet, IncomingDamage)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
};
