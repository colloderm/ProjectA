// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLChoiceArea.h"

#include "Components/ArrowComponent.h"

AMLChoiceArea::AMLChoiceArea()
{
	PrimaryActorTick.bCanEverTick = false;

	AreaRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AreaRoot"));
	RootComponent = AreaRoot;

	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(AreaRoot);
	Arrow->SetHiddenInGame(true);
	Arrow->SetRelativeRotation(FRotator(0.f, 45.f, 0.f)); // 화면 위쪽 (카메라 yaw 45) = 포털이 나오는 쪽
	Arrow->ArrowSize = 3.f;
}
