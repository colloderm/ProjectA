// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MLGE_Damage.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UMLGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMLGE_Damage();

	// 네이티브 태그 대신 FName: CDO는 네이티브 태그 등록이 보장되기 전에 만들어짐
	static const FName DamageName;
};
