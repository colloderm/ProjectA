// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLGameplayTags.h"

namespace MLTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Dash,		"Ability.Dash",			"Hero dash ability");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable,	"State.Invulnerable",	"Ignore incoming damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Dash,		"Cooldown.Dash",		"Granted by dash cooldown GE");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Cooldown,		"Data.Cooldown",		"SetByCaller: cooldown seconds");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead,			"State.Dead",			"Hero is dead; stops input and enemies");
}