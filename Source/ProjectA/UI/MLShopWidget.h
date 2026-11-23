// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLShopWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UMLRunProgressComponent;
class UMLShopEntryWidget;

// design §6.2.1 / §9-3 원정 상점: 골드 + 탐욕 줄, 구매 / 판매 탭, 탭의 아이템마다 행 하나, 나가기 버튼.
UCLASS(Abstract)
class PROJECTA_API UMLShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UMLRunProgressComponent* InRunProgress);
	void Refresh();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	// "골드 120 · 탐욕 1/4"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	// 현재 탭 버튼은 비활성화되어 선택됨을 나타냄.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SellTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ItemList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UMLShopEntryWidget> RowClass;

private:
	UFUNCTION()
	void HandleExit();

	UFUNCTION()
	void HandleBuyTab();

	UFUNCTION()
	void HandleSellTab();

	void SetTab(bool bSell);

	UPROPERTY(Transient)
	TObjectPtr<UMLRunProgressComponent> RunProgress;

	// 풀링된 행: 갱신마다 재사용되므로 행이 자기 클릭 안에서 파괴되지 않음.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLShopEntryWidget>> Rows;

	bool bSellTab = false;

	static constexpr float RowSpacing = 6.f;
};
