// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGuildGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/MLGuildHubWidget.h"

AMLGuildGameMode::AMLGuildGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultPawnClass = nullptr; // 메뉴 전용: 길드에는 영웅이 없음
}

void AMLGuildGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !HubWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ML] GuildGameMode: no player controller or HubWidgetClass not set"));
		return;
	}

	HubWidget = CreateWidget<UMLGuildHubWidget>(PC, HubWidgetClass);
	if (!HubWidget) return;

	HubWidget->AddToViewport();
	PC->SetInputMode(FInputModeUIOnly());
	PC->SetShowMouseCursor(true);
}
