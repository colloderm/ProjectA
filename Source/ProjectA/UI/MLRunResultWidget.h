// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLRunResultWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class UMLDamageRowWidget;
struct FMLRunSummary;

UCLASS(Abstract)
class PROJECTA_API UMLRunResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FMLRunSummary& Summary);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;
	
	// design §6 HUD 피해 리포트 (M9-0): 소스별 원정 합계
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RunDamageTitle;

	// 디자이너에서는 빈 VerticalBox
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RunDamageList;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TSubclassOf<UMLDamageRowWidget> DamageRowClass;

private:
	UFUNCTION()
	void HandleRestart();
};
