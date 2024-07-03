// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "Component/InventoryComponent.h"

#include "TPSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATPSPlayerController();

	virtual void Tick(float DeltaSeconds) override;

	/** Called after this PlayerController's viewport/net connection is associated with this player controller. */
	virtual void ReceivedPlayer() override;  //与服务器连接完成后就调用，在这里调用同步时间是最早的

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetServerTime();
	
protected:
	//同步服务器和客户端的时间(不是直接同步WorldTime，而是记录两者的差值，让客户端自己加)
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//将服务器收到请求时的WorldTime返还给对应客户端
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	void CheckTimeSync(float DeltaTime);
public:
	//背包组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UInventoryComponent* InventoryComponent;

protected:
	float ClientServerDelta = 0.f; //客户端当前时间和服务器时间的差值

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;  //距离上次与服务器同步时间过去了多久
};
