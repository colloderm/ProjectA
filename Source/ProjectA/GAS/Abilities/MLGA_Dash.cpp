// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGA_Dash.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "GameFramework/Character.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/MLGameplayTags.h"

UMLGA_Dash::UMLGA_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(MLTags::Ability_Dash.GetTag()));
	ActivationOwnedTags.AddTag(MLTags::State_Invulnerable.GetTag()); // 무적 시간 = 어빌리티가 활성인 동안
}

void UMLGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const ACharacter* Hero = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Hero || DashDuration <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector DashDir = Hero->GetLastMovementInputVector().GetSafeNormal2D();
	if (DashDir.IsNearlyZero())
	{
		DashDir = Hero->GetActorForwardVector();
	}

	UAbilityTask_ApplyRootMotionConstantForce* ForceTask =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, NAME_None, DashDir,
			DashDistance / DashDuration, // 세기 = 속도 (uu/s)
			DashDuration,
			false,      // bIsAdditive: 대시 중 걷기를 덮어씀
			nullptr,    // StrengthOverTime
			ERootMotionFinishVelocityMode::ClampVelocity,
			FVector::ZeroVector,
			600.f,      // 끝날 때 걷기 속도로 제한
			false);     // bEnableGravity

	if (!ForceTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ForceTask->OnFinish.AddDynamic(this, &UMLGA_Dash::HandleDashFinished);
	ForceTask->ReadyForActivation();
}

void UMLGA_Dash::HandleDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UMLGA_Dash::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) return;

	const float Reduction = ActorInfo->AbilitySystemComponent->GetNumericAttribute(
		UMLAttributeSet::GetDashCooldownReductionAttribute());

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(MLTags::Data_Cooldown.GetTag(), BaseCooldown * (1.f - Reduction));
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}
