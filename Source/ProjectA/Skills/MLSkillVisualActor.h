// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MLSkillVisualActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class PROJECTA_API AMLSkillVisualActor : public AActor
{
	GENERATED_BODY()

public:
	AMLSkillVisualActor();

	// design §6.2.3 망령의 기술 표시: BP에 추가한 메시를 포함해 모든 메시에 적용
	void SetOverlay(UMaterialInterface* Overlay);

private:
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> VisualMesh;
};
