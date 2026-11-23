// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MLListEntryWidget.h"
#include "MLFamilyWidget.generated.h"

class UPanelWidget;
class URichTextBlock;
class UTextBlock;

// design §8.3 / §9 #6 가문: 포인트 + 명성, 다음 등급 해금, 가문 강화 (즉시 구매), 유산 재능.
UCLASS(Abstract)
class PROJECTA_API UMLFamilyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

protected:
	// "가문 포인트 N · 명성 N"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PointsText;

	// 다음 잠긴 계약 등급과 필요한 명성
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> UnlockText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> UpgradeList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> TalentText;

	UPROPERTY(EditDefaultsOnly, Category = "Guild UI")
	TSubclassOf<UMLListEntryWidget> UpgradeClass;

private:
	void HandleUpgrade(int32 Index);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMLListEntryWidget>> UpgradeEntries;

	TArray<FName> UpgradeIds; // 목록 순서대로의 DT_FamilyUpgrades 행, Refresh마다 다시 만듦
};
