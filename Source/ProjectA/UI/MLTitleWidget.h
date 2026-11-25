// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MLListEntryWidget.h"
#include "MLTitleWidget.generated.h"

class UButton;
class UPanelWidget;
class URichTextBlock;

// design §8.7 칭호 창: [획득] [미획득] 탭. 획득 항목을 누르면 바로 장착 (창은 열린 채 강조만 바뀜), 미획득은 진행도만.
UCLASS(Abstract)
class PROJECTA_API UMLTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open();
	void Close();
	void Refresh(); // 허브 갱신(OnGuildChanged)마다 호출; 닫혀 있으면 아무것도 안 함

protected:
	virtual void NativeOnInitialized() override;

	// "{용병}의 칭호: {칭호}"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ViewTabList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> TitleList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> ViewTabClass;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> EntryClass;

private:
	UFUNCTION()
	void HandleClose();

	void HandleViewTab(int32 TabIndex);
	void HandleEntry(int32 EntryIndex);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> ViewTabs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> Entries;

	bool bLockedView = false;
	TArray<FName> ShownIds; // 항목 인덱스 → 칭호 ID (획득 탭: 0 = 칭호 없음 = NAME_None)
};
