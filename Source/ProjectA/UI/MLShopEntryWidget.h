// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLShopEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UMLRunProgressComponent;

// 상점 행 하나 (버튼 + 텍스트). ItemIndex는 원정 진행 상점 아이템을 가리킴.
UCLASS(Abstract)
class PROJECTA_API UMLShopEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UMLRunProgressComponent* InRunProgress, int32 InItemIndex);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EntryText;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient)
	TObjectPtr<UMLRunProgressComponent> RunProgress;

	int32 ItemIndex = INDEX_NONE;
};
