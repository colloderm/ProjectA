// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Guild/MLGuildTypes.h"
#include "Styling/SlateTypes.h"
#include "MLGuildHubWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;
class UMLConfirmWidget;
class UMLContractWidget;
class UMLMercWidget;
class UMLStashWidget;
class UMLFamilyWidget;
class UMLHallWidget;
class UMLTitleWidget;

// 열린 확인 창이 선택 시 할 일
enum class EMLGuildPending : uint8
{
	None,
	Contract,
	Sell,
	RetireTalent,
	RetireReplace,
	SellItem,
	BuyItem,
	Sortie,  // 선택지 = 의뢰 진열 인덱스 (마지막 선택지 = 의뢰 없음)
	Notice   // 안내 전용 (비정상 종료 복구)
};

// design §9 #2 길드 허브: 자원 바, 메뉴, 한 번에 하나의 내용 패널, 확인 창, 출정.
UCLASS(Abstract)
class PROJECTA_API UMLGuildHubWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// "골드 N · 명성 N · 가문 포인트 N"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResourceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContractTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MercTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StashTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> FamilyTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HallTabButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ContractTabText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MercTabText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StashTabText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FamilyTabText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HallTabText;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	FSlateColor MenuSelectedTint = FLinearColor(0.55f, 0.36f, 0.06f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	FSlateColor MenuSelectedTextColor = FLinearColor(0.06f, 0.04f, 0.01f, 1.f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SortieButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SortieHintText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLContractWidget> ContractPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLMercWidget> MercPanel;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLStashWidget> StashPanel;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLFamilyWidget> FamilyPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLHallWidget> HallPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLConfirmWidget> ConfirmPanel;

	// design §8.7 칭호 창 (용병 화면의 [칭호 변경])
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UMLTitleWidget> TitlePanel;

private:
	void Refresh();
	void ShowPanel(UWidget* Panel); // 표시 중인 패널의 메뉴 버튼은 선택된 모양 (여전히 클릭 가능)
	void SetMenuSelected(UButton* Button, UTextBlock* Label, bool bSelected);

	UFUNCTION()
	void HandleContractTab();

	UFUNCTION()
	void HandleMercTab();
	
	UFUNCTION()
	void HandleStashTab();
	
	UFUNCTION()
	void HandleFamilyTab();

	UFUNCTION()
	void HandleHallTab();

	UFUNCTION()
	void HandleSortie();
	
	void StartSortie();

	void HandleContractRequested(EMLGrade Grade, int32 IndexInGrade);
	void HandleRetireRequested();
	void HandleSellRequested();
	void HandleSellItemRequested(int32 StashIndex);
	void HandleBuyRequested(int32 OfferIndex);
	void HandleTitleRequested();
	void HandleConfirmChoice(int32 Choice);

	FDelegateHandle GuildChangedHandle;
	EMLGuildPending Pending = EMLGuildPending::None;
	EMLGrade PendingGrade = EMLGrade::F;
	int32 PendingIndex = INDEX_NONE; // 계약: 등급 내 인덱스 / 은퇴 교체: 재능 인덱스 / 아이템 판매: 보관함 인덱스 / 구매: 진열 인덱스
	UPROPERTY(Transient)
	FButtonStyle MenuStyle;

	UPROPERTY(Transient)
	FSlateColor MenuTextColor;

	UPROPERTY(Transient)
	FSlateFontInfo MenuFont;
};
