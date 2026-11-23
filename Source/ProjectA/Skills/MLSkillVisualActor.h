// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLSkillVisualActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PROJECTA_API AMLSkillVisualActor : public AActor
{
	GENERATED_BODY()

public:
	AMLSkillVisualActor();

private:
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
};
