// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SkillComponent.h"

#include "AbilitySystemComponent.h"
#include "SCharacter.h"
#include "TPSPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/TPSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
USkillComponent::USkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	//PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	MyOwnerPlayer = Cast<ASCharacter>(GetOwner());

	if(!SkillPercentWidgetClass)
	{
		//从C++中获取蓝图类
		const FString SkillPercentWidgetClassLoadPath = FString(TEXT("/Game/UI/CircleUI/UltimateChargeCircleView.UltimateChargeCircleView_C"));//蓝图一定要加_C这个后缀名
		SkillPercentWidgetClass = LoadClass<UUserWidget>(nullptr, *SkillPercentWidgetClassLoadPath);
	}
	if(SkillPercentWidgetClass)
	{
		if(MyOwnerPlayer)
		{
			ATPSPlayerController* MyController = Cast<ATPSPlayerController>(MyOwnerPlayer->GetController());
			if(MyController)  //服务端RestartPlayer时先触发BeginPlay然后OnPossess，BeginPlay时Controller暂时为空，所以在PossessedBy中再触发一次设置UI
			{
				MyController->ResetHUDWidgets(EHUDViewType::SkillPercentView);
			}
		}
	}
	
	if(GetOwnerRole()==ROLE_Authority)
	{
		ATPSGameMode* GM = Cast<ATPSGameMode>(UGameplayStatics::GetGameMode(this));
		if(MyOwnerPlayer && GM)
		{
			GM->OnGameBegin.AddDynamic(this, &USkillComponent::LearnAllSkills);
			LearnAllSkills(GM->GetHasGameBegun());
		}
	}

	SetSkillChargePercent(0);
}

// Called every frame
void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USkillComponent::SetSkillChargePercent_Implementation(float NewSkillChargePercent)
{
	const float TempPercent = FMath::Clamp(NewSkillChargePercent,0.f,100.f);
	
	if(SkillChargePercent == TempPercent) return;
	
	SkillChargePercent = TempPercent;
	/*if(GetOwnerRole() == ROLE_Authority)
	{
		//OnRep_SkillChargePercent(SkillChargePercent);
	}*/
	OnSkillPercentChanged.Broadcast(SkillChargePercent);
}

void USkillComponent::AddSkillChargePercent_Implementation(float AddPercent)
{
	SetSkillChargePercent(GetSkillChargePercent() + AddPercent);
}

bool USkillComponent::ActivateAbility(float AbilityIndex)
{
	if(MyOwnerPlayer && MyOwnerPlayer->GetAbilitySystemComponent() && GameplayTagContainer.IsValidIndex(AbilityIndex))
	{
		return MyOwnerPlayer->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(GameplayTagContainer[AbilityIndex]);
	}
	return false;
}

void USkillComponent::LearnAllSkills(bool HasGameBegun)
{
	if(HasGameBegun && MyOwnerPlayer && MyOwnerPlayer->GetAbilitySystemComponent())
	{
		if(UltimateSkillClass)
		{
			MyOwnerPlayer->GetAbilitySystemComponent()->GiveAbility(UltimateSkillClass);
		}
	}
}

void USkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USkillComponent, SkillChargePercent);
}

