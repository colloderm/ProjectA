// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLDamageRowWidget.generated.h"

class UPanelWidget;
class UProgressBar;
class UTextBlock;
struct FMLDamageLine;

// 피해 리포트의 소스 한 줄 (design §6 HUD, M9-0). WBP_DamageRow.
UCLASS(Abstract)
class PROJECTA_API UMLDamageRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLine(const FMLDamageLine& Line, float TotalDamage, float CombatSeconds);

	// 줄마다 행 하나, 주어진 큰 순서대로. 행은 재사용되고 남는 행은 접힘. 디자이너에서 목록은 비어 있어야 함.
	static void FillList(UUserWidget* Owner, UPanelWidget* List, TSubclassOf<UMLDamageRowWidget> RowClass,
		const TArray<FMLDamageLine>& Lines, float CombatSeconds);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	// "12,340 · 45% · DPS 257"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	// 전체 중 점유율
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ShareBar;
};
