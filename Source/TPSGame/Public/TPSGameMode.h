// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnd, int, WinnerID, int, WinningTeamID);
/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	//GameMode存在于服务器，不用加Server关键字
	void RespawnPlayer(APlayerController* PlayerController);

	void PlayerLeaveGame();

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
	int WinningTeamID;

	UPROPERTY(BlueprintReadWrite)
	bool bIsTeamMatchMode;

protected:
	UPROPERTY(BlueprintReadOnly)
	class ATPSGameState* MyGameState;
};
