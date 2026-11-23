// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;

namespace MLCombat
{
	// design §7.2: ATK x 배율 x (0.9~1.1), 치명타 x1.6
	float RollHeroDamage(const UAbilitySystemComponent& ASC, float DamageMult);
}