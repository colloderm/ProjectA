// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MLListEntryWidget.h"
#include "MLMercWidget.generated.h"

class UButton;
class UPanelWidget;
class URichTextBlock;

// design §9 #4 용병 정보: 스탯, 레벨, 재능, 주무기, 장비 (M8-2 전까지 읽기 전용), 기록, 은퇴 / 판매.
UCLASS(Abstract)
class PROJECTA_API UMLMercWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

	FSimpleDelegate OnRetireRequested; // 허브가 확인 창을 엶
	FSimpleDelegate OnSellRequested;
	FSimpleDelegate OnTitleRequested;  // design §8.7: 허브가 칭호 목록 창을 엶

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> MercText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RetireButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SellButton;

	// design §8.7 [칭호 변경]
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TitleButton;
	
	// design §7.5 훈련 행 (WBP_GuildTraining)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TrainingList;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> TrainingClass;

private:
	UFUNCTION()
	void HandleRetire();

	UFUNCTION()
	void HandleSell();

	UFUNCTION()
	void HandleTitle();
	
	void HandleTraining(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> TrainingEntries;

	TArray<FName> TrainingIds; // 목록 순서대로의 DT_Trainings 행, Refresh마다 다시 만듦
};
