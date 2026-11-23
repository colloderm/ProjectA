// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MLLevelUpWidget.generated.h"

class UButton;
class UTextBlock;
class URichTextBlock;
class UMLRunProgressComponent;

UCLASS(Abstract)
class PROJECTA_API UMLLevelUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void Setup(UMLRunProgressComponent* InRunProgress);
    void Refresh();
    void FocusFirstChoice(APlayerController* PC);

protected:
    virtual void NativeOnInitialized() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ChoiceButton0;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ChoiceButton1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ChoiceButton2;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<URichTextBlock> ChoiceText0;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<URichTextBlock> ChoiceText1;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<URichTextBlock> ChoiceText2;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RerollButton;
    
    // 선택 항목: "레벨 업!" / "모닥불"
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TitleText;

    // 카드 버튼 배경색 (M9-1a-2). 버튼 스타일 Tint는 밝게 두고 이 값으로 색을 정함. 글자는 밝은 색 기준
    UPROPERTY(EditAnywhere, Category = "Tier")
    FLinearColor DefaultCardTint = FLinearColor(0.08f, 0.08f, 0.1f, 1.f); // 1단계 · 패시브 · 회복

    UPROPERTY(EditAnywhere, Category = "Tier")
    FLinearColor Tier2CardTint = FLinearColor(0.28f, 0.12f, 0.5f, 1.f);

    UPROPERTY(EditAnywhere, Category = "Tier")
    FLinearColor Tier3CardTint = FLinearColor(0.5f, 0.36f, 0.06f, 1.f);

private:
    UFUNCTION()
    void HandleChoice0();

    UFUNCTION()
    void HandleChoice1();

    UFUNCTION()
    void HandleChoice2();

    UFUNCTION()
    void HandleReroll();

    UPROPERTY(Transient)
    TObjectPtr<UMLRunProgressComponent> RunProgress;
};
