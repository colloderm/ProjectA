// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Run/MLPassiveTypes.h"
#include "MLGE_Passive.generated.h"

class UAbilitySystemComponent;

UCLASS()
class PROJECTA_API UMLGE_Passive : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMLGE_Passive();

	// 모든 스탯을 중립값으로 채운 뒤 Stat에 Value를 설정.
	static FGameplayEffectSpecHandle MakeSpec(const UAbilitySystemComponent& ASC, EMLPassiveStat Stat, float Value);
};
