// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSGameType/Team.h"
#include "TPSGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnd, int, WinnerID, ETeam, WinningTeam);
/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATPSGameMode();
	
	virtual void SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	//GameMode存在于服务器，不用加Server关键字
	void RespawnPlayer(APlayerController* PlayerController);

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage);
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, Category="Game")
	virtual bool ReadyToEndMatch_Implementation() override;
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnMatchEnd OnMatchEnd;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int WinThreshold = 5;

	UPROPERTY(BlueprintReadWrite)
	int WinnerID;

	UPROPERTY(BlueprintReadWrite)
	ETeam WinningTeam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsTeamMatchMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TeammateDamage)
	bool bCanAttackTeammate = false;  //是否开启友军伤害
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TeammateDamage)
	float TeammateDamageRate;  //友军伤害倍率
	
protected:
	UPROPERTY(BlueprintReadOnly)
	class ATPSGameState* MyGameState;
};
