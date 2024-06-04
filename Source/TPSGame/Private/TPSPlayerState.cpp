// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerState.h"
#include "SCharacter.h"

#include "Net/UnrealNetwork.h"

void ATPSPlayerState::Reset()
{
	Super::Reset();
}

void ATPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSPlayerState, PersonalScore);
	DOREPLIFETIME(ATPSPlayerState, RankInGame);
	DOREPLIFETIME(ATPSPlayerState, Kills);
	DOREPLIFETIME(ATPSPlayerState, Deaths);
	DOREPLIFETIME(ATPSPlayerState, TeamID);
	DOREPLIFETIME(ATPSPlayerState, ItemList);
}

void ATPSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ASCharacter* Player = Cast<ASCharacter>(GetInstigator());
	if(Player && Player->GetInventoryComponent())
	{
		if(Player->GetInventoryComponent()->ItemList.Num() > 0)
		{
			ItemList = Player->GetInventoryComponent()->ItemList;
			ItemList.Shrink();
		}
		else
		{
			ItemList.Empty();
		}
	}
}
