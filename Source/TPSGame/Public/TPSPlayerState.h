// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Component/InventoryComponent.h"
#include "TPSPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

protected:
	virtual void CopyProperties(APlayerState* PlayerState) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int PersonalScore = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int RankInGame = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int Kills = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int Deaths = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int TeamID = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<FMyItem> ItemList;
};
