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

	virtual float CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage) override;
	
protected:
	virtual void HandleMatchHasStarted() override;

private:
	void TryAddAndSetTeam(APlayerState* NewPlayerState);
};
