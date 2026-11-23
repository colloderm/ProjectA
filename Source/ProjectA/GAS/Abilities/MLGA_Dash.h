// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MLGA_Dash.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UMLGA_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMLGA_Dash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 쿨타임(초) = BaseCooldown * (1 - DashCooldownReduction), SetByCaller Data.Cooldown으로 전달
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

private:
	UFUNCTION()
	void HandleDashFinished();

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDistance = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float BaseCooldown = 1.5f;
};
