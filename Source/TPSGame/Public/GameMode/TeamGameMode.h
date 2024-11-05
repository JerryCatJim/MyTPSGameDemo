// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPSGameMode.h"
#include "TeamGameMode.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATeamGameMode : public ATPSGameMode
{
	GENERATED_BODY()

public:
	ATeamGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	virtual void RestartPlayer(AController* NewPlayer) override;
	
	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
	
protected:
	virtual void HandleMatchHasStarted() override;

private:
	void TryAddAndSetTeam(APlayerState* NewPlayerState);

private:
	//客户端刚进入游戏时先触发了RestartPlayer然后才触发Login(?)，所以把玩家分配到对应队伍出生点不会成功，
	//记录设置失败的玩家，等Login后调用TryAddAndSetTeam时再重新RestartPlayer一次
	//客户端触发顺序为 : RestartPlayer -> HandleMatchHasStarted -> PostLogin (???)
	UPROPERTY()
	TArray<AController*> SetTeamFailedControllers;
};
