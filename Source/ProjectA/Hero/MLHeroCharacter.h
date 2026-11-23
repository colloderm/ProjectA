// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MLHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAbilitySystemComponent;
class UMLAttributeSet;
class UGameplayAbility;
class UInputMappingContext;
class UInputAction;
class UMLBasicAttackComponent;
class UMLRunProgressComponent;
class UWidgetComponent;
struct FInputActionValue;

UCLASS()
class PROJECTA_API AMLHeroCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMLHeroCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// Health가 0이 되면 UMLAttributeSet이 한 번 호출
	void HandleDeath();
	bool IsDead() const;
	
	UMLRunProgressComponent* GetRunProgress() const { return RunProgress; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleDash();
	void HandleRegen();

	FTimerHandle RegenTimerHandle;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> HeroASC;

	UPROPERTY()
	TObjectPtr<UMLAttributeSet> HeroAttributes;
	
	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UMLBasicAttackComponent> BasicAttack;
	
	UPROPERTY(VisibleAnywhere, Category = "Run")
	TObjectPtr<UMLRunProgressComponent> RunProgress;
	
	// 머리 위 HP + 쉴드. Widget Class = WBP_HeroBars (BP_Hero에서 설정)
	UPROPERTY(VisibleAnywhere, Category = "UI")
	TObjectPtr<UWidgetComponent> HeroBars;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DashAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;
};
