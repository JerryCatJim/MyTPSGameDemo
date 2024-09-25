// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TeamGameMode.h"
#include "TPSGameState.h"
#include "TPSPlayerState.h"
#include "Kismet/GameplayStatics.h"

ATeamGameMode::ATeamGameMode()
{
	bIsTeamMatchMode = true;
}

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if(MyGameState)
	{
		TryAddAndSetTeam(NewPlayer->PlayerState);
	}
}

void ATeamGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	if(MyGameState)
	{
		ATPSPlayerState* LeavePS = Exiting->GetPlayerState<ATPSPlayerState>();
		if(LeavePS)
		{
			if(MyGameState->BlueTeam.Contains(LeavePS))
			{
				MyGameState->BlueTeam.Remove(LeavePS);
			}
			else if(MyGameState->RedTeam.Contains(LeavePS))
			{
				MyGameState->RedTeam.Remove(LeavePS);
			}
		}
	}
}

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
	if(MyGameState)
	{
		for(APlayerState* PS : MyGameState->PlayerArray)
		{
			TryAddAndSetTeam(PS);
		}
	}
}

void ATeamGameMode::TryAddAndSetTeam(APlayerState* NewPlayerState)
{
	if(!NewPlayerState) return;
	
	ATPSPlayerState* MyPS = Cast<ATPSPlayerState>(NewPlayerState);
	if(MyPS && MyPS->GetTeam() == ETeam::ET_NoTeam)
	{
		if(MyGameState->BlueTeam.Num() >= MyGameState->RedTeam.Num())
		{
			MyGameState->RedTeam.AddUnique(MyPS);
			MyPS->SetTeam(ETeam::ET_RedTeam);
		}
		else
		{
			MyGameState->BlueTeam.AddUnique(MyPS);
			MyPS->SetTeam(ETeam::ET_BlueTeam);
		}
	}
}
