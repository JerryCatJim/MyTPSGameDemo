// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TPSPlayerState.h"
#include "TPSGameState.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSGameState : public AGameState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SortPlayerRank_Stable(UPARAM(ref)TArray<FPlayerDataInGame>& PlayerDataArray, bool HighToLow = true);
};
