// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MLGuildGameMode.generated.h"

class UMLGuildHubWidget;

// design §9 #2 길드 허브 맵: 폰 없음, 허브 UI만.
UCLASS()
class PROJECTA_API AMLGuildGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMLGuildGameMode();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Guild")
	TSubclassOf<UMLGuildHubWidget> HubWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMLGuildHubWidget> HubWidget;
};
