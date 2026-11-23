// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLRoomGateActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/MLGateLabelWidget.h"

AMLRoomGateActor::AMLRoomGateActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GateRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GateRoot"));
	SetRootComponent(GateRoot);

	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(GateRoot);
	GateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 생성자 안, GateMesh 설정 바로 아래
	LabelWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LabelWidget"));
	LabelWidget->SetupAttachment(GateRoot);
	LabelWidget->SetWidgetSpace(EWidgetSpace::Screen); // 항상 카메라를 향함, 크기 고정, UMG가 한글을 렌더링
	LabelWidget->SetDrawAtDesiredSize(true);
	LabelWidget->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
	LabelWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMLRoomGateActor::InitGate(EMLRoomType InRoomType, bool bInExitDoor)
{
	RoomType = InRoomType;
	bExitDoor = bInExitDoor;
	SetChannelProgress(0.f);
}

void AMLRoomGateActor::SetChannelProgress(float Ratio)
{
	if (Ratio <= 0.f)
	{
		ApplyLabel(GetBaseLabel());
		return;
	}
	ApplyLabel(FText::Format(NSLOCTEXT("ML", "GateChannel", "{0}  {1}%"),
		GetBaseLabel(), FText::AsNumber(FMath::Min(100, FMath::RoundToInt(Ratio * 100.f)))));
}

void AMLRoomGateActor::ApplyLabel(const FText& Text)
{
	// 위젯은 컴포넌트의 BeginPlay에서 만들어지며, 이는 SpawnActor 안에서 InitGate보다 먼저 실행됨.
	if (UMLGateLabelWidget* Label = Cast<UMLGateLabelWidget>(LabelWidget->GetUserWidgetObject()))
	{
		Label->SetLabel(Text);
	}
}

FText AMLRoomGateActor::GetBaseLabel() const
{
	return bExitDoor ? NSLOCTEXT("ML", "ExitDoor", "탈출 문") : MLRun::GetRoomTypeName(RoomType);
}
