// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "MLRunProgressComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GAS/Effects/MLGE_Passive.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/MLGameplayTags.h"
#include "Guild/MLGuildSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MLLevelUpWidget.h"
#include "UI/MLRunHUDWidget.h"
#include "UI/MLShopWidget.h"
#include "Skills/MLAutoSkill.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	// 기술 단계 표시 (M9-1a-2): ★ / ★★ / ★★★
	FText TierStars(int32 Tier)
	{
		return FText::FromString(FString::ChrN(FMath::Clamp(Tier, 1, 3), TCHAR(0x2605)));
	}
}

UMLRunProgressComponent::UMLRunProgressComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMLRunProgressComponent::InitForPlayer(APlayerController* PC)
{
	if (!PC || !PC->IsLocalController() || !HUDWidgetClass || HUDWidget) return;

	HUDWidget = CreateWidget<UMLRunHUDWidget>(PC, HUDWidgetClass);
	if (!HUDWidget) return;

	HUDWidget->InitHUD(this);
	HUDWidget->AddToViewport();
}

void UMLRunProgressComponent::AddXP(float Amount)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (Amount <= 0.f || !ASC || ASC->HasMatchingGameplayTag(MLTags::State_Dead.GetTag())) return;

	RunXP += Amount * (1.f + ASC->GetNumericAttribute(UMLAttributeSet::GetXPGainAttribute()) / 100.f);
	while (RunXP >= GetXPToNext())
	{
		RunXP -= GetXPToNext(); // ++ 전에 현재 레벨의 요구량을 사용
		++RunLevel;
		++PendingLevelUps;
	}

	OnProgressChanged.Broadcast();

	if (PendingLevelUps > 0 && !bLevelUpOpen)
	{
		OpenLevelUp();
	}
}

void UMLRunProgressComponent::AddGold(int32 Amount)
{
	if (Amount <= 0) return;

	RunGold += Amount;
	OnProgressChanged.Broadcast(); // HUD 골드 / 탐욕
}

void UMLRunProgressComponent::AddChest(int32 Step)
{
	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UMLGuildSubsystem* Guild = GI ? GI->GetSubsystem<UMLGuildSubsystem>() : nullptr;
	int32 Tier = 0;
	const FName ItemId = Guild ? Guild->RollItem(Step, Tier) : NAME_None;
	if (ItemId.IsNone()) return; // ItemTable 미설정

	RunItems.Add(ItemId);
	RunItemTierTotal += Tier;
	OnProgressChanged.Broadcast(); // HUD 아이템 수 / 탐욕
}

FText UMLRunProgressComponent::GetChoiceText(int32 ChoiceIndex) const
{
	if (!Choices.IsValidIndex(ChoiceIndex)) return FText::GetEmpty();

	const FText ChoiceFormat = NSLOCTEXT("ML", "PassiveChoice", "<Name>{0}</>  <Dim>Lv{1}</>\n<Desc>{2}</>");
	const FMLLevelUpChoice& Choice = Choices[ChoiceIndex];
	if (Choice.RestAction != EMLRestAction::None) return GetRestChoiceText(Choice);

	if (Choice.SkillClass && Choice.bCombo)
	{
		const UMLAutoSkill* ComboDefaults = Choice.SkillClass->GetDefaultObject<UMLAutoSkill>();
		const TArray<TSubclassOf<UMLAutoSkill>>& Inputs = ComboDefaults->GetRecipeInputs();
		const FText InputA = Inputs.IsValidIndex(0) && Inputs[0] ? Inputs[0]->GetDefaultObject<UMLAutoSkill>()->GetSkillName() : FText::GetEmpty();
		const FText InputB = Inputs.IsValidIndex(1) && Inputs[1] ? Inputs[1]->GetDefaultObject<UMLAutoSkill>()->GetSkillName() : FText::GetEmpty();
		return FText::Format(NSLOCTEXT("ML", "ComboChoice", "<Talent>조합</>  <Name>{0} {1}</>\n<Dim>{2} + {3}</>\n<Desc>{4}</>"),
			TierStars(ComboDefaults->GetSkillTier()), ComboDefaults->GetSkillName(), InputA, InputB, ComboDefaults->GetSkillDescription());
	}

	if (Choice.SkillClass)
	{
		const UMLAutoSkill* SkillDefaults = Choice.SkillClass->GetDefaultObject<UMLAutoSkill>();
		const FText StarredName = FText::Format(NSLOCTEXT("ML", "StarredName", "{0} {1}"), TierStars(SkillDefaults->GetSkillTier()), SkillDefaults->GetSkillName());
		return FText::Format(ChoiceFormat, StarredName, FText::AsNumber(Choice.NextLevel), SkillDefaults->GetSkillDescription());
	}

	const FMLPassiveRow* Row = (PassiveTable && !Choice.PassiveRow.IsNone())
		? PassiveTable->FindRow<FMLPassiveRow>(Choice.PassiveRow, TEXT("MLRunProgress"))
		: nullptr;
	if (!Row) return FText::Format(NSLOCTEXT("ML", "HealChoice", "<Name>회복</>\n<Desc>HP {0} 회복</>"), FText::AsPercent(FallbackHealPercent));

	return FText::Format(ChoiceFormat, Row->DisplayName, FText::AsNumber(Choice.NextLevel), Row->Description);
}

void UMLRunProgressComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!bLevelUpOpen || !IsChoiceEnabled(ChoiceIndex)) return;

	const FMLLevelUpChoice Picked = Choices[ChoiceIndex];
	if (bRestMode)
	{
		ApplyRestChoice(Picked);
		bRestMode = false;
		OnSlotsChanged.Broadcast();
		CloseLevelUp();
		OnRestFinished.Broadcast();
		return;
	}
	
	ApplyLevelUpChoice(Picked);
	
	OnSlotsChanged.Broadcast();

	if (--PendingLevelUps > 0)
	{
		RollChoices();
		RefreshOpenWidget();
	}
	else
	{
		CloseLevelUp();
	}
}

void UMLRunProgressComponent::Reroll()
{
	if (!bLevelUpOpen || !CanReroll()) return;

	bRerollUsed = true; // design §6.4: 원정마다 무료 다시 뽑기 1회
	RollChoices();
	RefreshOpenWidget();
}

void UMLRunProgressComponent::OpenLevelUp()
{
	RollChoices();
	if (!ShowChoiceWidget())
	{
		PendingLevelUps = 0; // UI 미설정: 레벨은 오르지만 선택지는 건너뜀
	}
}

bool UMLRunProgressComponent::ShowChoiceWidget()
{
	APlayerController* PC = GetOwningPC();
	if (!PC || !LevelUpWidgetClass) return false;

	if (!LevelUpWidget)
	{
		LevelUpWidget = CreateWidget<UMLLevelUpWidget>(PC, LevelUpWidgetClass);
		if (!LevelUpWidget) return false;
		LevelUpWidget->Setup(this);
	}

	LevelUpWidget->AddToViewport(10);
	bLevelUpOpen = true;

	PC->SetInputMode(FInputModeUIOnly());
	PC->SetShowMouseCursor(true);
	UGameplayStatics::SetGamePaused(this, true);

	RefreshOpenWidget();
	return true;
}

void UMLRunProgressComponent::CloseLevelUp()
{
	bLevelUpOpen = false;
	if (LevelUpWidget)
	{
		LevelUpWidget->RemoveFromParent();
	}

	if (APlayerController* PC = GetOwningPC())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	UGameplayStatics::SetGamePaused(this, false);
}

void UMLRunProgressComponent::RefreshOpenWidget()
{
	if (!LevelUpWidget) return;

	LevelUpWidget->Refresh();
	LevelUpWidget->FocusFirstChoice(GetOwningPC());
}

void UMLRunProgressComponent::RollChoices()
{
	Choices.Reset();

	// design §6.4.1 (M9-1): 조합할 수 있으면 조합 카드 1장을 고정 (다시 뽑기에도 유지)
	TArray<FMLLevelUpChoice> Combos;
	for (const TSubclassOf<UMLAutoSkill>& ComboClass : ComboSkillPool)
	{
		if (!CanCombo(ComboClass)) continue;
		FMLLevelUpChoice& Combo = Combos.AddDefaulted_GetRef();
		Combo.SkillClass = ComboClass;
		Combo.NextLevel = 1;
		Combo.bCombo = true;
	}
	TArray<FMLLevelUpChoice> Fixed;
	if (Combos.Num() > 0)
	{
		// 가능한 조합이 여럿이면 (예: 오라 조합 둘) 하나만 무작위로
		Fixed.Add(Combos[FMath::RandRange(0, Combos.Num() - 1)]);
	}

	TArray<FMLLevelUpChoice> Candidates;
	GatherCandidates(Candidates);

	// 빈 기술 슬롯 + 얻을 수 있는 1단계 기술이 있으면 새 기술 카드 1장 고정 (M9-1a-2)
	TArray<int32> NewSkillIndices;
	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		if (Candidates[i].SkillClass && Candidates[i].NextLevel == 1) NewSkillIndices.Add(i);
	}
	if (NewSkillIndices.Num() > 0)
	{
		const int32 Pick = NewSkillIndices[FMath::RandRange(0, NewSkillIndices.Num() - 1)];
		Fixed.Add(Candidates[Pick]);
		Candidates.RemoveAtSwap(Pick);
	}

	const int32 RegularCount = ChoicesPerLevel - Fixed.Num();
	while (Candidates.Num() > 0 && Choices.Num() < RegularCount)
	{
		const int32 Pick = FMath::RandRange(0, Candidates.Num() - 1);
		Choices.Add(Candidates[Pick]);
		Candidates.RemoveAtSwap(Pick);
	}

	// design §6.4: 회복 하나로 채움 (중복 없음); 모두 최대면 회복만
	if (Choices.Num() < RegularCount)
	{
		Choices.Add(FMLLevelUpChoice());
	}

	for (const FMLLevelUpChoice& FixedChoice : Fixed)
	{
		Choices.Insert(FixedChoice, FMath::RandRange(0, Choices.Num()));
	}
}

int32 UMLRunProgressComponent::GetChoiceTier(int32 ChoiceIndex) const
{
	if (!Choices.IsValidIndex(ChoiceIndex) || !Choices[ChoiceIndex].SkillClass) return 0;

	const FMLLevelUpChoice& Choice = Choices[ChoiceIndex];
	if (Choice.RestAction == EMLRestAction::Evolve) return 3; // 진화 결과 = 3단계
	return Choice.SkillClass->GetDefaultObject<UMLAutoSkill>()->GetSkillTier();
}

// 레벨업과 상점이 공유: 슬롯이 비어 있으면 새 패시브/기술, 보유 중이면 다음 레벨.
void UMLRunProgressComponent::GatherCandidates(TArray<FMLLevelUpChoice>& OutCandidates) const
{
	if (PassiveTable)
	{
		const bool bHasFreePassiveSlot = PassiveLevels.Num() < MaxPassiveSlots;
		for (const FName& RowName : PassiveTable->GetRowNames())
		{
			const FMLPassiveRow* Row = PassiveTable->FindRow<FMLPassiveRow>(RowName, TEXT("MLRunProgress"));
			if (!Row) continue;

			const int32 OwnedLevel = PassiveLevels.FindRef(RowName);
			if (OwnedLevel > 0 ? OwnedLevel < Row->MaxLevel : bHasFreePassiveSlot)
			{
				OutCandidates.Add(FMLLevelUpChoice{ RowName, OwnedLevel + 1 });
			}
		}
	}

	// 보유 기술 레벨업: 2단계도 포함되므로 풀이 아니라 보유 목록에서
	for (const TObjectPtr<UMLAutoSkill>& Skill : OwnedSkills)
	{
		if (Skill && !Skill->IsMaxLevel())
		{
			OutCandidates.Add(FMLLevelUpChoice{ NAME_None, Skill->GetSkillLevel() + 1, Skill->GetClass() });
		}
	}

	// 새 1단계 기술: 빈 슬롯이 있고, 미보유이며, 조합 재료로 쓰지 않은 것
	if (OwnedSkills.Num() >= MaxSkillSlots) return;
	for (const TSubclassOf<UMLAutoSkill>& SkillClass : AutoSkillPool)
	{
		if (SkillClass && !FindOwnedSkill(SkillClass) && !ConsumedSkills.Contains(SkillClass))
		{
			OutCandidates.Add(FMLLevelUpChoice{ NAME_None, 1, SkillClass });
		}
	}
}

bool UMLRunProgressComponent::CanCombo(TSubclassOf<UMLAutoSkill> ComboClass) const
{
	const UMLAutoSkill* ComboDefaults = ComboClass ? ComboClass->GetDefaultObject<UMLAutoSkill>() : nullptr;
	if (!ComboDefaults || ComboDefaults->GetRecipeInputs().Num() != 2 || FindOwnedSkill(ComboClass)) return false;

	for (const TSubclassOf<UMLAutoSkill>& Input : ComboDefaults->GetRecipeInputs())
	{
		const UMLAutoSkill* Owned = Input ? FindOwnedSkill(Input) : nullptr;
		if (!Owned || !Owned->IsMaxLevel()) return false;
	}
	return true;
}

void UMLRunProgressComponent::ApplyCombo(TSubclassOf<UMLAutoSkill> ComboClass)
{
	if (!CanCombo(ComboClass)) return;

	// 2단계는 두 재료 중 앞쪽 슬롯에 들어가고, 한 칸이 빔
	const TArray<TSubclassOf<UMLAutoSkill>> Inputs = ComboClass->GetDefaultObject<UMLAutoSkill>()->GetRecipeInputs();
	int32 SlotIndex = OwnedSkills.Num();
	for (const TSubclassOf<UMLAutoSkill>& Input : Inputs)
	{
		SlotIndex = FMath::Min(SlotIndex, OwnedSkills.IndexOfByPredicate([&Input](const TObjectPtr<UMLAutoSkill>& Skill)
		{
			return Skill && Skill->GetClass() == Input;
		}));
	}
	for (const TSubclassOf<UMLAutoSkill>& Input : Inputs)
	{
		RemoveSkill(Input);
		ConsumedSkills.Add(Input);
	}

	UMLAutoSkill* Combo = NewObject<UMLAutoSkill>(this, ComboClass);
	OwnedSkills.Insert(Combo, FMath::Clamp(SlotIndex, 0, OwnedSkills.Num()));
	Combo->Acquire(GetOwner());
}

void UMLRunProgressComponent::ApplyPassive(FName RowName)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	const FMLPassiveRow* Row = PassiveTable ? PassiveTable->FindRow<FMLPassiveRow>(RowName, TEXT("MLRunProgress")) : nullptr;
	if (!ASC || !Row) return;

	const FGameplayEffectSpecHandle SpecHandle = UMLGE_Passive::MakeSpec(*ASC, Row->Stat, Row->ValuePerLevel);
	if (SpecHandle.IsValid())
	{
		PassiveEffects.FindOrAdd(RowName).Add(ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));
		++PassiveLevels.FindOrAdd(RowName);
	}
}

void UMLRunProgressComponent::HealPercent(float Percent)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC) return;

	const float MaxHP = ASC->GetNumericAttribute(UMLAttributeSet::GetMaxHealthAttribute());
	const float CurrentHP = ASC->GetNumericAttribute(UMLAttributeSet::GetHealthAttribute());
	const float HealAmount = FMath::Min(MaxHP * Percent, MaxHP - CurrentHP);
	if (HealAmount > 0.f)
	{
		ASC->ApplyModToAttribute(UMLAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, HealAmount);
	}
}

APlayerController* UMLRunProgressComponent::GetOwningPC() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

void UMLRunProgressComponent::ApplySkill(TSubclassOf<UMLAutoSkill> SkillClass)
{
	if (UMLAutoSkill* Owned = FindOwnedSkill(SkillClass))
	{
		Owned->LevelUp();
		return;
	}

	UMLAutoSkill* NewSkill = NewObject<UMLAutoSkill>(this, SkillClass);
	OwnedSkills.Add(NewSkill);
	NewSkill->Acquire(GetOwner());
}

UMLAutoSkill* UMLRunProgressComponent::FindOwnedSkill(TSubclassOf<UMLAutoSkill> SkillClass) const
{
	for (const TObjectPtr<UMLAutoSkill>& Skill : OwnedSkills)
	{
		// 정확한 클래스 일치: 2·3단계 BP는 1단계 BP의 자식이라 IsA면 1단계 카드가 2단계를 올려 버림 (M9-1)
		if (Skill && Skill->GetClass() == SkillClass)
		{
			return Skill;
		}
	}
	return nullptr;
}

const FMLPassiveRow* UMLRunProgressComponent::FindPassiveRow(FName RowName) const
{
	return PassiveTable ? PassiveTable->FindRow<FMLPassiveRow>(RowName, TEXT("MLRunProgress")) : nullptr;
}

void UMLRunProgressComponent::OpenRestChoices()
{
	Choices.Reset();

	FMLLevelUpChoice Heal;
	Heal.RestAction = EMLRestAction::Heal;
	Choices.Add(Heal);

	FMLLevelUpChoice Evolve;
	Evolve.RestAction = EMLRestAction::Evolve;
	if (const UMLAutoSkill* Evolvable = FindEvolvableSkill())
	{
		Evolve.SkillClass = Evolvable->GetClass(); // shortcut: 조건을 만족하는 첫 번째만; 나머지는 다음 휴식터를 기다림
	}
	Evolve.bEnabled = Evolve.SkillClass != nullptr;
	Choices.Add(Evolve);

	TArray<const UMLAutoSkill*> Upgradable;
	for (const TObjectPtr<UMLAutoSkill>& Skill : OwnedSkills)
	{
		if (Skill && !Skill->IsMaxLevel()) Upgradable.Add(Skill.Get());
	}
	FMLLevelUpChoice Upgrade;
	Upgrade.RestAction = EMLRestAction::Upgrade;
	if (Upgradable.Num() > 0)
	{
		const UMLAutoSkill* Pick = Upgradable[FMath::RandRange(0, Upgradable.Num() - 1)];
		Upgrade.SkillClass = Pick->GetClass();
		Upgrade.NextLevel = Pick->GetSkillLevel() + 1;
	}
	Upgrade.bEnabled = Upgrade.SkillClass != nullptr;
	Choices.Add(Upgrade);

	bRestMode = true;
	if (!ShowChoiceWidget())
	{
		bRestMode = false;
		OnRestFinished.Broadcast(); // UI 미설정: 모닥불을 건너뜀
	}
}

FText UMLRunProgressComponent::GetRestChoiceText(const FMLLevelUpChoice& Choice) const
{
	const UMLAutoSkill* Skill = Choice.SkillClass ? FindOwnedSkill(Choice.SkillClass) : nullptr;
	switch (Choice.RestAction)
	{
	case EMLRestAction::Evolve:
		if (Skill && Skill->GetEvolvedClass())
		{
			const UMLAutoSkill* Evolved = Skill->GetEvolvedClass()->GetDefaultObject<UMLAutoSkill>();
			return FText::Format(NSLOCTEXT("ML", "RestEvolve", "<Name>기술 진화</>\n{0} → <Talent>{1} {2}</>\n<Desc>{3}</>"),
				Skill->GetSkillName(), TierStars(Evolved->GetSkillTier()), Evolved->GetSkillName(), Evolved->GetSkillDescription());
		}
		return NSLOCTEXT("ML", "RestEvolveNone", "<Name>기술 진화</>\n<Dim>진화 조건: 2단계 기술 Lv3 + 지정 패시브</>");

	case EMLRestAction::Upgrade:
		if (Skill)
		{
			return FText::Format(NSLOCTEXT("ML", "RestUpgrade", "<Name>기술 강화</>\n{0}  <Dim>Lv{1}</>"), Skill->GetSkillName(), FText::AsNumber(Choice.NextLevel));
		}
		return NSLOCTEXT("ML", "RestUpgradeNone", "<Name>기술 강화</>\n<Dim>강화할 기술 없음</>");

	default:
		return FText::Format(NSLOCTEXT("ML", "RestHeal", "<Name>회복</>\n<Desc>HP {0} 회복</>"), FText::AsPercent(RestHealPercent));
	}
}

void UMLRunProgressComponent::ApplyRestChoice(const FMLLevelUpChoice& Picked)
{
	UMLAutoSkill* Skill = Picked.SkillClass ? FindOwnedSkill(Picked.SkillClass) : nullptr;

	switch (Picked.RestAction)
	{
	case EMLRestAction::Evolve:
		EvolveSkill(Skill);
		break;
	case EMLRestAction::Upgrade:
		if (Skill) Skill->LevelUp();
		break;
	default:
		HealPercent(RestHealPercent);
		break;
	}
}

UMLAutoSkill* UMLRunProgressComponent::FindEvolvableSkill() const
{
	for (const TObjectPtr<UMLAutoSkill>& Skill : OwnedSkills)
	{
		const TSubclassOf<UMLAutoSkill> Target = Skill ? Skill->GetEvolvedClass() : nullptr;
		// 진화 BP는 부모의 EvolvedClass(= 자기 자신)를 물려받음; 같은 클래스로는 절대 진화하지 않음
		// 2단계만 진화 (M9-1): 1단계 BP에 예전 EvolvedClass가 남아 있어도 무시
		if (Target && Target != Skill->GetClass() && Skill->GetSkillTier() == 2 && Skill->IsMaxLevel()
			&& PassiveLevels.Contains(Skill->GetEvolvePassiveRow()))
		{
			return Skill;
		}
	}
	return nullptr;
}

void UMLRunProgressComponent::EvolveSkill(UMLAutoSkill* OldSkill)
{
	const int32 SlotIndex = OldSkill ? OwnedSkills.IndexOfByKey(OldSkill) : INDEX_NONE;
	if (SlotIndex == INDEX_NONE || !OldSkill->GetEvolvedClass()) return;

	UMLAutoSkill* Evolved = NewObject<UMLAutoSkill>(this, OldSkill->GetEvolvedClass());
	OldSkill->Release();
	OwnedSkills[SlotIndex] = Evolved; // 같은 슬롯 (design §6.4.1)
	Evolved->Acquire(GetOwner());
}

void UMLRunProgressComponent::ApplyLevelUpChoice(const FMLLevelUpChoice& Picked)
{
	if (Picked.SkillClass && Picked.bCombo)
	{
		ApplyCombo(Picked.SkillClass);
	}
	else if (Picked.SkillClass)
	{
		ApplySkill(Picked.SkillClass);
	}
	else if (!Picked.PassiveRow.IsNone())
	{
		ApplyPassive(Picked.PassiveRow);
	}
	else
	{
		HealPercent(FallbackHealPercent);
	}
}

void UMLRunProgressComponent::SetMenuMode(bool bMenu)
{
	if (APlayerController* PC = GetOwningPC())
	{
		if (bMenu)
		{
			PC->SetInputMode(FInputModeUIOnly());
		}
		else
		{
			PC->SetInputMode(FInputModeGameOnly());
		}
		PC->SetShowMouseCursor(bMenu);
	}
	UGameplayStatics::SetGamePaused(this, bMenu);
}

void UMLRunProgressComponent::OpenShop(float PriceMult)
{
	ShopPriceMult = PriceMult;
	ShopStock.Reset();

	TArray<FMLLevelUpChoice> Candidates;
	GatherCandidates(Candidates);
	for (int32 i = 0; i < ShopOfferCount && Candidates.Num() > 0; ++i)
	{
		const int32 Pick = FMath::RandRange(0, Candidates.Num() - 1);
		FMLShopItem& Offer = ShopStock.AddDefaulted_GetRef();
		Offer.Kind = EMLShopItemKind::Offer;
		Offer.Target = Candidates[Pick];
		Offer.Price = FMath::RoundToInt((ShopBasePrice + ShopLevelPrice * (Offer.Target.NextLevel - 1)) * PriceMult);
		Candidates.RemoveAtSwap(Pick);
	}

	FMLShopItem& Potion = ShopStock.AddDefaulted_GetRef();
	Potion.Kind = EMLShopItemKind::Potion;
	Potion.Price = FMath::RoundToInt(ShopPotionPrice * PriceMult);

	RebuildShopItems();

	APlayerController* PC = GetOwningPC();
	if (!PC || !ShopWidgetClass)
	{
		OnShopFinished.Broadcast(); // UI 미설정: 상점을 건너뜀
		return;
	}
	if (!ShopWidget)
	{
		ShopWidget = CreateWidget<UMLShopWidget>(PC, ShopWidgetClass);
		if (!ShopWidget)
		{
			OnShopFinished.Broadcast();
			return;
		}
		ShopWidget->Setup(this);
	}

	ShopWidget->AddToViewport(10);
	SetMenuMode(true);
	ShopWidget->Refresh();
}

void UMLRunProgressComponent::CloseShop()
{
	if (!ShopWidget || !ShopWidget->IsInViewport()) return;

	ShopWidget->RemoveFromParent();
	SetMenuMode(false);

	OnShopFinished.Broadcast();
}

void UMLRunProgressComponent::RebuildShopItems()
{
	ShopItems = ShopStock;

	// design §6.2.1 판매 행: Price = 받는 골드
	const float SellPerLevel = ShopSellPerLevel * ShopPriceMult;
	for (const TObjectPtr<UMLAutoSkill>& Skill : OwnedSkills)
	{
		if (!Skill) continue;
		FMLShopItem& Item = ShopItems.AddDefaulted_GetRef();
		Item.Kind = EMLShopItemKind::SellSkill;
		Item.Target.SkillClass = Skill->GetClass();
		Item.Target.NextLevel = Skill->GetSkillLevel();
		// 단계별 판매 레벨 (M9-1): 1단계 = 레벨, 2단계 = 재료 몫 + 레벨, 3단계 = 고정
		const int32 Tier = Skill->GetSkillTier();
		const int32 SellLevel = Tier >= 3 ? ShopSellTier3Level : (Tier == 2 ? ShopSellTier2Base + Skill->GetSkillLevel() : Skill->GetSkillLevel());
		Item.Price = FMath::RoundToInt(SellPerLevel * SellLevel);
	}
	for (const TPair<FName, int32>& Pair : PassiveLevels)
	{
		FMLShopItem& Item = ShopItems.AddDefaulted_GetRef();
		Item.Kind = EMLShopItemKind::SellPassive;
		Item.Target.PassiveRow = Pair.Key;
		Item.Target.NextLevel = Pair.Value;
		Item.Price = FMath::RoundToInt(SellPerLevel * Pair.Value);
	}
}

bool UMLRunProgressComponent::IsOfferStillValid(const FMLLevelUpChoice& Offer) const
{
	// 상점을 연 뒤 다른 구매나 제거로 슬롯이 찼거나 레벨이 바뀌었을 수 있음.
	if (Offer.SkillClass)
	{
		const UMLAutoSkill* Owned = FindOwnedSkill(Offer.SkillClass);
		return Owned ? Owned->GetSkillLevel() + 1 == Offer.NextLevel : (Offer.NextLevel == 1 && OwnedSkills.Num() < MaxSkillSlots);
	}
	const int32 OwnedLevel = PassiveLevels.FindRef(Offer.PassiveRow);
	return OwnedLevel > 0 ? OwnedLevel + 1 == Offer.NextLevel : (Offer.NextLevel == 1 && PassiveLevels.Num() < MaxPassiveSlots);
}

bool UMLRunProgressComponent::IsShopSellItem(int32 Index) const
{
	return ShopItems.IsValidIndex(Index)
		&& (ShopItems[Index].Kind == EMLShopItemKind::SellSkill || ShopItems[Index].Kind == EMLShopItemKind::SellPassive);
}

bool UMLRunProgressComponent::IsShopItemEnabled(int32 Index) const
{
	if (!ShopItems.IsValidIndex(Index)) return false;

	const FMLShopItem& Item = ShopItems[Index];
	if (Item.Kind == EMLShopItemKind::SellSkill) return OwnedSkills.Num() > 1; // 영웅은 기술로만 공격함
	if (Item.Kind == EMLShopItemKind::SellPassive) return true;

	if (Item.bSold || RunGold < Item.Price) return false;
	return Item.Kind != EMLShopItemKind::Offer || IsOfferStillValid(Item.Target);
}

FText UMLRunProgressComponent::GetTargetName(const FMLLevelUpChoice& Target) const
{
	if (Target.SkillClass)
	{
		return Target.SkillClass->GetDefaultObject<UMLAutoSkill>()->GetSkillName();
	}
	const FMLPassiveRow* Row = FindPassiveRow(Target.PassiveRow);
	return Row ? Row->DisplayName : FText::FromName(Target.PassiveRow);
}

FText UMLRunProgressComponent::GetShopItemText(int32 Index) const
{
	if (!ShopItems.IsValidIndex(Index)) return FText::GetEmpty();

	const FMLShopItem& Item = ShopItems[Index];
	if (IsShopSellItem(Index))
	{
		return FText::Format(NSLOCTEXT("ML", "ShopSell", "{0}  Lv{1}    +{2}G"),
			GetTargetName(Item.Target), FText::AsNumber(Item.Target.NextLevel), FText::AsNumber(Item.Price));
	}

	const FText Label = Item.Kind == EMLShopItemKind::Potion
		? FText::Format(NSLOCTEXT("ML", "ShopPotion", "회복약 (HP {0})"), FText::AsPercent(FallbackHealPercent))
		: FText::Format(NSLOCTEXT("ML", "ShopOffer", "{0}  Lv{1}{2}"), GetTargetName(Item.Target), FText::AsNumber(Item.Target.NextLevel),
			Item.Target.NextLevel == 1 ? NSLOCTEXT("ML", "ShopNew", " (신규)") : FText::GetEmpty());

	return FText::Format(NSLOCTEXT("ML", "ShopItem", "{0}    {1}G{2}"), Label, FText::AsNumber(Item.Price),
		Item.bSold ? NSLOCTEXT("ML", "ShopSold", "  [품절]") : FText::GetEmpty());
}

void UMLRunProgressComponent::TradeShopItem(int32 Index)
{
	if (!IsShopItemEnabled(Index)) return;

	const FMLShopItem Item = ShopItems[Index];
	RunGold += IsShopSellItem(Index) ? Item.Price : -Item.Price; // design §6.2.2: 탐욕은 골드를 바로 따라감

	switch (Item.Kind)
	{
	case EMLShopItemKind::Offer:       ApplyLevelUpChoice(Item.Target); break;
	case EMLShopItemKind::Potion:      HealPercent(FallbackHealPercent); break;
	case EMLShopItemKind::SellSkill:   RemoveSkill(Item.Target.SkillClass); break;
	case EMLShopItemKind::SellPassive: RemovePassive(Item.Target.PassiveRow); break;
	}
	if (ShopStock.IsValidIndex(Index)) // 판매 행은 ShopStock 뒤에 있으므로 이것은 구매만 표시
	{
		ShopStock[Index].bSold = true;
	}

	RebuildShopItems();
	OnProgressChanged.Broadcast(); // HUD 골드 / 탐욕
	OnSlotsChanged.Broadcast();    // HUD 슬롯
	if (ShopWidget) ShopWidget->Refresh();
}

void UMLRunProgressComponent::RemoveSkill(TSubclassOf<UMLAutoSkill> SkillClass)
{
	const int32 SlotIndex = OwnedSkills.IndexOfByPredicate([&SkillClass](const TObjectPtr<UMLAutoSkill>& Skill)
	{
		return Skill && Skill->GetClass() == SkillClass;
	});
	if (SlotIndex == INDEX_NONE) return;

	OwnedSkills[SlotIndex]->Release();
	OwnedSkills.RemoveAt(SlotIndex);
}

void UMLRunProgressComponent::RemovePassive(FName RowName)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC) return;

	TArray<FActiveGameplayEffectHandle> Handles;
	PassiveEffects.RemoveAndCopyValue(RowName, Handles);
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	PassiveLevels.Remove(RowName);

	// 최대 HP 패시브가 사라짐: 현재 HP를 새 최대치 안으로 유지.
	const float MaxHP = ASC->GetNumericAttribute(UMLAttributeSet::GetMaxHealthAttribute());
	if (ASC->GetNumericAttribute(UMLAttributeSet::GetHealthAttribute()) > MaxHP)
	{
		ASC->SetNumericAttributeBase(UMLAttributeSet::GetHealthAttribute(), MaxHP);
	}
}
