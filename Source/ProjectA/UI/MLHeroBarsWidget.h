// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Guild/MLGuildTypes.h"
#include "MLHeroBarsWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
struct FOnAttributeChangeData;

// 머리 위 HP + 쉴드 바, AMLHeroCharacter::HeroBars가 표시 (스크린 공간).
UCLASS(Abstract)
class PROJECTA_API UMLHeroBarsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitBars(UAbilitySystemComponent* ASC);

	// design §8.7: 장착 칭호를 등급 색으로 표시. 이름이 비면 숨김
	void SetTitle(const FText& TitleName, EMLTitleRarity Rarity);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ShieldBar;

	// design §8.7 칭호 (HP 바 위). 없으면 칭호를 표시하지 않음
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	// 칭호 등급 색: 일반(흰) / 희귀(파랑) / 영웅(보라) / 전설(금)
	UPROPERTY(EditAnywhere, Category = "Title")
	TArray<FLinearColor> TitleColors = { FLinearColor(0.92f, 0.92f, 0.92f), FLinearColor(0.35f, 0.65f, 1.f), FLinearColor(0.72f, 0.45f, 1.f), FLinearColor(1.f, 0.78f, 0.25f) };
	
	// design §6.4: 바가 차오르는 시간(초) (내려갈 때는 즉시)
	UPROPERTY(EditDefaultsOnly, Category = "Bars", meta = (ClampMin = "0.01"))
	float FillSeconds = 0.4f;

private:
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void RefreshBars();
	void SetBarTarget(float& Shown, float Target, float& FillRate) const;
	void ApplyBars();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	// 표시 전용 값 (0..1); 어트리뷰트는 즉시 바뀌고 바가 따라감
	float ShownHealth = -1.f;		// < 0 = 아직 아무것도 표시 안 함: 첫 유효값은 바로 맞춤 (원정 시작 시 채우기 없음)
	float TargetHealth = 0.f;
	float HealthFillRate = 0.f;
	float ShownShield = 0.f;
	float TargetShield = 0.f;
	float ShieldFillRate = 0.f;
	float ShieldPeak = 0.f;			// 마지막으로 받은 쉴드 양 = 쉴드 바의 100%
};
