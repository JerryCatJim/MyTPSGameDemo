// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Component/InventoryComponent.h"
#include "TPSPlayerController.generated.h"

UENUM(BlueprintType)
enum class EHUDViewType : uint8
{
	AllViews,
	CrossHairView,
	SkillPercentView,
};

class ASCharacter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLagDetected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLagEnded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerLeaveGame);
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
	
	UFUNCTION(Server, Reliable)
	void RequestRespawn();

	UFUNCTION(BlueprintCallable)
	void ResetHUDWidgets(EHUDViewType HUDViewType);
	
	UFUNCTION(BlueprintCallable)
	void ResetCrossHair();
	UFUNCTION(BlueprintCallable)
	void RemoveCrossHair();
	UFUNCTION(BlueprintCallable)
	void SetCrossHairVisibility(bool IsVisible);

	UFUNCTION(BlueprintCallable)
	void ResetSkillPercentWidget();
	
	//UFUNCTION(Server, Reliable)
	void PlayerLeaveGame();

	UFUNCTION(BlueprintCallable, Server, Reliable)  //必须加Server才能调用到GameState中(???)
	void RefreshScoreBoardUI();

	UFUNCTION(BlueprintCallable)
	float GetRespawnCount();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetNewPawn(TSubclassOf<ASCharacter> NewPawnClass);

	TSubclassOf<APawn> GetNewPawnClassToPossess() const { return NewPawnClassToPossess; }
	
protected:
	virtual void SetupInputComponent() override;

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;
	
	void ShowReturnToMainMenu();
	
	//同步服务器和客户端的时间(不是直接同步WorldTime，而是记录两者的差值，让客户端自己加)
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	//将服务器收到请求时的WorldTime返还给对应客户端
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	void CheckTimeSync(float DeltaTime);

	void CheckLag();

	//OnLagDetected和OnLagEnd委托在蓝图WBP_TPSGameMainView中绑定，此时OnLagDetected可能已经广播过，所以绑定完成后重设一次计时器并发送广播
	UFUNCTION(BlueprintCallable)
	void AskForLagSituation();
private:
	UFUNCTION()
	void LagDetectedBroadcast();
	UFUNCTION()
	void LagEndedBroadcast();
	
public:
	//背包组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UInventoryComponent* InventoryComponent;
	
	UPROPERTY(BlueprintAssignable)
	FOnPlayerLeaveGame OnPlayerLeaveGame;
	
protected:
	float ClientServerDelta = 0.f; //客户端当前时间和服务器时间的差值

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;  //距离上次与服务器同步时间过去了多久

	FTimerHandle FShowLagIconHandle;

	UPROPERTY(EditAnywhere, Category=NetworkDelayCheck)  //延迟超过多少时判定为卡顿
	float HighPingThreshold = 150.f;

	UPROPERTY(EditAnywhere, Category=NetworkDelayCheck)  //延迟过高时，每隔多少秒显示一次Wifi闪烁动画以提醒卡顿
	float HighPingLoopDuration = 10.f;

	UPROPERTY(BlueprintAssignable)
	FOnLagDetected OnLagDetected;
	UPROPERTY(BlueprintAssignable)
	FOnLagDetected OnLagEnded;
	
	//角色死亡后重生的计时器句柄
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	FTimerHandle FPlayerRespawnTimerHandle;

private:
	UPROPERTY(EditAnywhere, Category=HUD)
	TSubclassOf<UUserWidget> ReturnToMainMenuClass;

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	UPROPERTY()
	TSubclassOf<APawn> NewPawnClassToPossess;
};