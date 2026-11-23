// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MLListEntryWidget.h"
#include "MLHallWidget.generated.h"

class UPanelWidget;
class UTextBlock;

// design §8.5 / §9 #7 명예의 전당: 떠난 모든 용병 (은퇴 / 판매 / 전사), 최신순, 읽기 전용.
UCLASS(Abstract)
class PROJECTA_API UMLHallWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> HallList;

	// 전당이 비어 있을 때만 표시
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> RowClass;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> HallRows;
};
