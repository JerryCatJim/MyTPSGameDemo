// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SHealthComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	//PrimaryComponentTick.bCanEverTick = true;

	//生命值初始化
	MaxHealth = 100.f;
	Health = MaxHealth;

	//开启网络复制，将该实体从服务器端Server复制到客户端Client
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if(GetOwnerRole() == ROLE_Authority)  //如果所属角色具有主控权（服务器）
	{
		AActor* MyOwner = GetOwner();
		if(MyOwner)
		{
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);  //Actor.h中自带的动态委托，受到伤害后OnTakeAnyDamage会Broadcast绑定的函数
		}
	}
	
}


// Called every frame
/*void USHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}*/

void USHealthComponent::HandleTakeAnyDamage_Implementation(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if(Damage <= 0.f )
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	//在(例如血条UI等)蓝图中绑定好事件，通过广播可以接收
	OnHealthChanged.Broadcast(this, Health, -Damage, DamageType, InstigatedBy, DamageCauser);
	
	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));
}

void USHealthComponent::OnRep_Health(float OldHealth)  //服务器通知其他客户端Client同步更新（这是客户端调用的函数）
{
	float Damage = Health - OldHealth;

	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

//恢复生命值
void USHealthComponent::Heal_Implementation(float HealAmount)
{
	if(HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.f, MaxHealth);
	UE_LOG(LogTemp, Log, TEXT("Health Increased: %s"), *FString::SanitizeFloat(HealAmount));

	OnHealthChanged.Broadcast(this, Health, HealAmount, nullptr, nullptr, nullptr);
}


//一个模板
void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	DOREPLIFETIME(USHealthComponent, Health);
}
