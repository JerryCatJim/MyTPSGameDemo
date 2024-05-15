// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/SBuffComponent.h"

#include "SPowerUpActor.h"

// Sets default values for this component's properties
USBuffComponent::USBuffComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
	//开启网络复制，将该实体从服务器端Server复制到客户端Client
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void USBuffComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void USBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

//每种道具只保留一个实例，方便在其内部使用计时器，计算叠加层数等操作
void USBuffComponent::AddToBuffList(int32 PowerUpID, ASPowerUpActor* PowerUpActor, AActor* OverlapActor)
{
	if(!PowerUpActor)
	{
		return;
	}

	ASPowerUpActor* TempActor = HashMap.FindRef(PowerUpID);
	//如果刷新了Buff效果
	if(IsValid(TempActor))  //如果道具实例还在则视为刷新
	{
		TempActor->ActivePowerUp(OverlapActor);  //在道具内部处理叠加层数，是否立刻刷新效果等逻辑
		//销毁新拾取的道具实体
		PowerUpActor->Destroy();
	}
	else //不存在时为未生效过，或者Buff持续效果结束后，此时键值还在，但是实例为空指针
	{
		//TMap键值唯一
		HashMap.Add(PowerUpID, PowerUpActor);
		PowerUpActor->ActivePowerUp(OverlapActor);
	}
}
