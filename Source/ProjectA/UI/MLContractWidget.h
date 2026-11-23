// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Guild/MLGuildTypes.h"
#include "MLContractWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UMLListEntryWidget;

DECLARE_DELEGATE_TwoParams(FMLOnContractRequested, EMLGrade /*Grade*/, int32 /*IndexInGrade*/);

// design §8.1 / §9 #3 계약소: 등급 탭, 선택한 등급의 후보 카드, 다시 뽑기.
UCLASS(Abstract)
class PROJECTA_API UMLContractWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

	FMLOnContractRequested OnContractRequested; // 허브가 확인 창을 엶

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> GradeTabList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CandidateList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RerollButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RerollText;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> TabClass;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> CardClass;

private:
	UFUNCTION()
	void HandleReroll();

	void HandleGradeTab(int32 GradeIndex);
	void HandleCard(int32 IndexInGrade);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> Tabs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> Cards;

	EMLGrade SelectedGrade = EMLGrade::F;
};
