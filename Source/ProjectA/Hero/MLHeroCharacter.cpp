// 프로젝트 설정의 Description 페이지에서 저작권 고지를 작성하세요.

#include "Hero/MLHeroCharacter.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/MLAttributeSet.h"
#include "GAS/MLGameplayTags.h"
#include "Guild/MLGuildSubsystem.h"
#include "InputActionValue.h"
#include "Combat/MLBasicAttackComponent.h"
#include "Run/MLRunProgressComponent.h"
#include "Run/MLRunGameMode.h"
#include "Components/WidgetComponent.h"
#include "UI/MLHeroBarsWidget.h"
#include "Engine/GameInstance.h"

namespace
{
	constexpr float RegenInterval = 0.5f; // design §6.4 재생 주기
}

AMLHeroCharacter::AMLHeroCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);
	MoveComp->MaxWalkSpeed = 600.f; // design §6.1: 모든 용병 고정
	MoveComp->bConstrainToPlane = true;
	MoveComp->bSnapToPlaneAtStart = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // 쿼터뷰는 영웅과 함께 회전하지 않음
	CameraBoom->SetRelativeRotation(FRotator(-55.f, 45.f, 0.f));
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	HeroASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("HeroASC"));
	
	HeroAttributes = CreateDefaultSubobject<UMLAttributeSet>(TEXT("HeroAttributes"));
	
	BasicAttack = CreateDefaultSubobject<UMLBasicAttackComponent>(TEXT("BasicAttack"));
	
	RunProgress = CreateDefaultSubobject<UMLRunProgressComponent>(TEXT("RunProgress"));
	
	HeroBars = CreateDefaultSubobject<UWidgetComponent>(TEXT("HeroBars"));
	HeroBars->SetupAttachment(RootComponent);
	HeroBars->SetWidgetSpace(EWidgetSpace::Screen); // 크기 고정, 적·이펙트 위에 그려짐
	HeroBars->SetDrawAtDesiredSize(true);
	HeroBars->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	HeroBars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UAbilitySystemComponent* AMLHeroCharacter::GetAbilitySystemComponent() const
{
	return HeroASC;
}

void AMLHeroCharacter::BeginPlay()
{
	Super::BeginPlay(); // HeroBars는 자기 BeginPlay 안에서 위젯을 만듦

	if (UMLHeroBarsWidget* BarsWidget = Cast<UMLHeroBarsWidget>(HeroBars->GetUserWidgetObject()))
	{
		BarsWidget->InitBars(HeroASC);
	}
	
	// design §6.4 체력 재생 패시브: 액터 틱 대신 타이머 (레벨업 중 게임과 함께 일시정지)
	GetWorldTimerManager().SetTimer(RegenTimerHandle, this, &AMLHeroCharacter::HandleRegen, RegenInterval, true);
}

void AMLHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	HeroASC->InitAbilityActorInfo(this, this);

	// design §7.2: 용병 스탯 → 원정 어트리뷰트, 원정마다 한 번 (길드 계층 → 원정 계층, 단방향)
	FMLStatBlock Stats;
	const UMLGuildSubsystem* Guild = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMLGuildSubsystem>() : nullptr;
	if (!Guild || !Guild->GetActiveStats(Stats))
	{
		// 용병 없음 (길드 테이블 미설정): 예전 고정 테스트 영웅
		Stats.HP = 100.f;
		Stats.ATK = 10.f;
		Stats.CRIT = 5.f;
	}
	const float DashCDR = FMath::Min(Stats.DODGE * 0.01f, 0.5f);
	const float DodgeChance = FMath::Min(Stats.DODGE * 0.2f, 10.f);
	const float SkillCDR = FMath::Min(Stats.SPECIAL * 0.005f, 0.4f);

	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetMaxHealthAttribute(), Stats.HP);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetHealthAttribute(), Stats.HP);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetDashCooldownReductionAttribute(), DashCDR);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetDodgeChanceAttribute(), DodgeChance);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetDefenseAttribute(), Stats.DEF);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetAttackAttribute(), Stats.ATK);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetAttackSpeedAttribute(), Stats.SPD);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetCritChanceAttribute(), Stats.CRIT);
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetPickupRadiusAttribute(), 300.f); // design §6.3 (M4 이후 조정)
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetXPGainAttribute(), 20.f); // 기본 원정 경험치 +20% (5+5 슬롯 기준 조정)
	HeroASC->SetNumericAttributeBase(UMLAttributeSet::GetSkillCooldownReductionAttribute(), SkillCDR);
	
	// design §7.4: 장착 무기, 없으면 용병 주무기가 기본 공격을 정함 (BP 기본 행 = dagger)
	float WeaponDamageScale = 1.f;
	const FName WeaponRow = Guild ? Guild->GetActiveWeaponRow(WeaponDamageScale) : NAME_None;
	if (!WeaponRow.IsNone()) BasicAttack->WeaponRowHandle.RowName = WeaponRow;
	BasicAttack->DamageScale = WeaponDamageScale;

	// 싱글 플레이: 원정마다 한 번만 빙의하므로 중복 부여 방지 없음.
	if (DashAbilityClass)
	{
		HeroASC->GiveAbility(FGameplayAbilitySpec(DashAbilityClass, 1));
	}
	
	RunProgress->InitForPlayer(Cast<APlayerController>(NewController));
}

void AMLHeroCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !DefaultMappingContext) return;

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AMLHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMLHeroCharacter::HandleMove);
	}
	if (DashAction)
	{
		EIC->BindAction(DashAction, ETriggerEvent::Started, this, &AMLHeroCharacter::HandleDash);
	}
}

void AMLHeroCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, CameraBoom->GetComponentRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRot);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), MoveInput.Y); // 화면 위쪽
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), MoveInput.X); // 화면 오른쪽
}

void AMLHeroCharacter::HandleDash()
{
	HeroASC->TryActivateAbilitiesByTag(FGameplayTagContainer(MLTags::Ability_Dash.GetTag()));
}

void AMLHeroCharacter::HandleRegen()
{
	if (IsDead()) return;
	
	// design §6.4: 방에서 전투 중일 때만 재생 (보석 수집·포털 선택·안전 방에서는 안 함)
	const AMLRunGameMode* RunMode = GetWorld()->GetAuthGameMode<AMLRunGameMode>();
	if (RunMode && RunMode->GetRunState() != EMLRunState::Combat) return;

	const float RegenPercent = HeroASC->GetNumericAttribute(UMLAttributeSet::GetHealthRegenAttribute());
	if (RegenPercent <= 0.f) return;

	const float MaxHP = HeroASC->GetNumericAttribute(UMLAttributeSet::GetMaxHealthAttribute());
	const float Missing = MaxHP - HeroASC->GetNumericAttribute(UMLAttributeSet::GetHealthAttribute());
	const float Heal = FMath::Min(MaxHP * RegenPercent * 0.01f * RegenInterval, Missing);
	if (Heal <= 0.f) return;

	HeroASC->ApplyModToAttribute(UMLAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, Heal);
}

void AMLHeroCharacter::HandleDeath()
{
	if (IsDead()) return;

	HeroASC->AddLooseGameplayTag(MLTags::State_Dead.GetTag());
	GetCharacterMovement()->DisableMovement();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// design §6.6: 용병 소실 / 명예의 전당은 FinishRun을 거쳐 UMLGuildSubsystem::EndRun이 적용
	UE_LOG(LogTemp, Warning, TEXT("[ML] Hero died"));
	if (AMLRunGameMode* RunMode = GetWorld()->GetAuthGameMode<AMLRunGameMode>())
	{
		RunMode->HandleHeroDied();
	}
}

bool AMLHeroCharacter::IsDead() const
{
	return HeroASC && HeroASC->HasMatchingGameplayTag(MLTags::State_Dead.GetTag());
}
