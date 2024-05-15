// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SPowerUpActor.generated.h"

//这是道具基类(可派生各种效果不同的道具)

UCLASS()
class TPSGAME_API ASPowerUpActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASPowerUpActor();

	void ActivePowerUp(AActor* OverlapActor);  //激活道具
	
	UFUNCTION(BlueprintImplementableEvent, Category = PowerUps)
	void OnActivated_BP(AActor* OverlapActor, int32 CurrentBuffLayer);  //自定义函数 道具激活时(蓝图中实现)

	UFUNCTION(BlueprintImplementableEvent, Category = PowerUps)
	void OnExpired_BP();  //自定义函数 道具失效时（蓝图中实现）

	void OnActivated(int32 CurrentBuffLayer);

	void OnExpired();

	int32 GetPowerUpID();  //获取道具唯一ID

	int32 GetCurrentOverlyingNum();  //获取当前Buff层数
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 道具持续时间(或间隔x秒生效N次)
	UFUNCTION()
	void OnPowerUpTicked();

	//刷新Buff状态
	void RefreshBuffState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_PowerUpActive();

	UFUNCTION(BlueprintImplementableEvent, Category= PowerUps)
	void OnActiveStateChanged_BP();
public:

protected:
	UPROPERTY(EditDefaultsOnly, Category= PowerUps)
	float PowerUpInterval;  //道具效果的持续时间

	UPROPERTY(EditDefaultsOnly, Category= PowerUps)
	int32 MaxOverlyingNum;  //道具Buff最大可叠加层数
	
	UPROPERTY(EditDefaultsOnly, Category= PowerUps)
	int32 SingleOverlyingNum;  //道具单次叠加层数

	UPROPERTY(EditDefaultsOnly, Category= PowerUps)
	bool bCanDecay;  //true为层层衰减，否则一次性掉光; 例如吃血药一次上4次回血Buff，掉一层回一次血

	UPROPERTY(EditDefaultsOnly, Category= PowerUps)
	bool bRefreshImmediately;  //获取道具效果后是否立刻刷新计时器进度条

	UPROPERTY(BlueprintReadOnly, Category= PowerUps)
	int32 CurrentOverlyingNum; //道具已叠加层数
	
	//计时器句柄
	FTimerHandle TimerHandle_PowerUpTick;

	UPROPERTY(EditAnywhere, Category= PowerUps)
	int32 PowerUpID;  //道具唯一ID

	UPROPERTY(ReplicatedUsing= OnRep_PowerUpActive, BlueprintReadOnly, Category= PowerUps)
	bool bIsActive;  //道具是否处于激活状态

	UPROPERTY(BlueprintReadOnly, Category= PowerUps)
	AActor* CurrentOverlapActor;    //用于记录拾取了该道具的Actor(Character)
};
