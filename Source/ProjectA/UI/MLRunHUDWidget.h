// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLRunHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UHorizontalBox;
class UMLRunProgressComponent;
class UMLSlotIconWidget;
class UMLDamageRowWidget;
class UPanelWidget;
class UWidget;
class AMLRunGameMode;

UCLASS(Abstract)
class PROJECTA_API UMLRunHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitHUD(UMLRunProgressComponent* InRunProgress);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> XPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RunLevelText;
	
	// "1구역 · 방 2/3 · 남은 적 12" / "… · 황금 슬라임 0:42" / "다음 방을 고르세요"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RoomText;

	// "골드 123 · 탐욕 2/4" (design §6.2.2)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;
	
	// 영웅 주위에서 열린 포털 쪽을 가리키는 화살표. 아무 위젯이나 가능; 그림은 오른쪽(0°)을 가리킴.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> PortalArrow;
	
	// 황금 방 슬라임 / 정예 방 가장 가까운 엘리트, 멀리 있을 때. PortalArrow와 같은 설정.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> GoblinArrow;
	
	// 보스 방 전용: 보스 이름과 HP 바가 있는 상단 가운데 패널 (design §6.5)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> BossPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BossBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	// 황금 방 전투 중에만 상단 가운데에 표시하는 강조 문구 (M9-0b)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ObjectiveText;

	// design §8.5.1 원정 중 의뢰 진행 (M9-5). 없으면 표시하지 않음
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RequestText;

	// 피해 없이 의뢰에서 이번 방에 맞았을 때 의뢰 줄 색
	UPROPERTY(EditAnywhere, Category = "Request")
	FLinearColor RequestFailColor = FLinearColor(1.f, 0.2f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Arrows")
	float ArrowShowDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Arrows")
	float ArrowRadius = 110.f;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 왼쪽 위 2줄: 위는 기술, 아래는 패시브
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> SkillRow;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> PassiveRow;

	UPROPERTY(EditDefaultsOnly, Category = "Slots")
	TSubclassOf<UMLSlotIconWidget> SlotIconClass;

	UPROPERTY(EditAnywhere, Category = "Slots")
	FLinearColor SkillSlotColor = FLinearColor(0.9f, 0.5f, 0.15f); // 1단계

	// 기술 단계별 슬롯 색 (M9-1a-2)
	UPROPERTY(EditAnywhere, Category = "Slots")
	FLinearColor Tier2SlotColor = FLinearColor(0.55f, 0.3f, 0.95f);

	UPROPERTY(EditAnywhere, Category = "Slots")
	FLinearColor Tier3SlotColor = FLinearColor(1.f, 0.75f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "Slots")
	FLinearColor PassiveSlotColor = FLinearColor(0.2f, 0.5f, 0.9f);

	UPROPERTY(EditAnywhere, Category = "Slots")
	FLinearColor EmptySlotColor = FLinearColor(0.f, 0.f, 0.f, 0.35f);
	
	// design §6 HUD 피해 리포트 (M9-0): 오른쪽, 전투 방 이후 보석 수집 / 포털 선택 중 표시
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DamagePanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageTitle;

	// 디자이너에서는 빈 VerticalBox; 행은 런타임에 생성
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> DamageList;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UMLDamageRowWidget> DamageRowClass;

private:
	void BuildRow(UHorizontalBox* Row, int32 SlotCount, TArray<TObjectPtr<UMLSlotIconWidget>>& OutSlots);
	void RefreshXP();
	void RefreshSlots();
	void RefreshRoom();
	void RefreshGold();
	void RefreshRequest();
	void RefreshDamagePanel();
	void UpdateArrow(UWidget* Arrow, bool bActive, const FVector& Target, bool& bShown);

	TWeakObjectPtr<AMLRunGameMode> BoundRunMode;
	bool bPortalArrowShown = false;
	bool bGoblinArrowShown = false;
	bool bBossPanelShown = false;
	bool bDamagePanelShown = false;
	bool bObjectiveShown = false;
	FSlateColor RequestDefaultColor; // 디자이너에서 정한 의뢰 줄 색, 실패 뒤 되돌리기용

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLSlotIconWidget>> SkillSlots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLSlotIconWidget>> PassiveSlots;

	TWeakObjectPtr<UMLRunProgressComponent> BoundRunProgress;

	static constexpr float SlotSpacing = 4.f;
};
