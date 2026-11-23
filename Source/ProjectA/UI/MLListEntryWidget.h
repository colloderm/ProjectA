// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "MLListEntryWidget.generated.h"

class UButton;
class UPanelWidget;
class URichTextBlock;

DECLARE_DELEGATE_OneParam(FMLOnEntryClicked, int32 /*EntryIndex*/);

// 모든 길드 목록용 범용 클릭 행 / 카드 (버튼 + 텍스트). 소유자가 풀링함.
UCLASS(Abstract)
class PROJECTA_API UMLListEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(int32 InEntryIndex, const FText& Text, bool bEnabled);
	
	// 선택됨 = SelectedTint. 아니면 BaseTint가 있으면 그것 (예: 아이템 티어), 없으면 WBP 자체 스타일.
	void SetSelected(bool bSelected, const FLinearColor* BaseTint = nullptr);

	// 생성 직후 소유자가 한 번만 바인딩, 다시 바인딩하지 않음: Setup이 이 항목의 클릭 안에서 실행될 수 있음.
	FMLOnEntryClicked OnEntryClicked;

	// Pool을 Count까지 늘리고 (새 항목은 Panel에) 나머지는 접음. EntryClass / Panel이 없으면 false.
	static bool SyncPool(UUserWidget* Owner, TSubclassOf<UMLListEntryWidget> EntryClass, UPanelWidget* Panel,
		TArray<TObjectPtr<UMLListEntryWidget>>& Pool, int32 Count);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> EntryText;
	
	// 선택 상태의 일반 / 호버 / 눌림 색
	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	FSlateColor SelectedTint = FLinearColor(0.55f, 0.36f, 0.06f, 1.f);

private:
	UFUNCTION()
	void HandleClicked();

	int32 EntryIndex = INDEX_NONE;
	
	UPROPERTY(Transient)
	FButtonStyle UnselectedStyle; // WBP 자체 스타일, NativeOnInitialized에서 한 번 저장
};
