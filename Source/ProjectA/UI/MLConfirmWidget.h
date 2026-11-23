// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MLListEntryWidget.h"
#include "MLConfirmWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class URichTextBlock;

// design §9 #13: 계약 / 판매 / 은퇴 공용 모달 하나. 선택지를 누르면 확정; 취소는 닫기.
UCLASS(Abstract)
class PROJECTA_API UMLConfirmWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(const FText& Title, const FText& Body, const TArray<FText>& Choices);
	void Close();

	FMLOnEntryClicked OnChoice; // Choices의 인덱스

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> BodyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ChoiceList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> ChoiceClass;

private:
	UFUNCTION()
	void HandleCancel();

	void HandleChoice(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> ChoiceEntries;
};
