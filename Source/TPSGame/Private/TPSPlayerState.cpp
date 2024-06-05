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

	DOREPLIFETIME(ATPSPlayerState, PlayerDataInGame);
}

void ATPSPlayerState::BeginPlay()
{
	Super::BeginPlay();
}

#pragma region GetterAndSetter
int ATPSPlayerState::GetPersonalScore() const
{
	return PlayerDataInGame.PersonalScore;
}

void ATPSPlayerState::SetPersonalScore(int PersonalScore)
{
	PlayerDataInGame.PersonalScore = PersonalScore;
}

int ATPSPlayerState::GetRankInGame() const
{
	return PlayerDataInGame.RankInGame;
}

void ATPSPlayerState::SetRankInGame(int RankInGame)
{
	PlayerDataInGame.RankInGame = RankInGame;
}

int ATPSPlayerState::GetKills() const
{
	return PlayerDataInGame.Kills;
}

void ATPSPlayerState::SetKills(int Kills)
{
	PlayerDataInGame.Kills = Kills;
}

int ATPSPlayerState::GetDeaths() const
{
	return PlayerDataInGame.Deaths;
}

void ATPSPlayerState::SetDeaths(int Deaths)
{
	PlayerDataInGame.Deaths = Deaths;
}

int ATPSPlayerState::GetTeamID() const
{
	return PlayerDataInGame.TeamID;
}

void ATPSPlayerState::SetTeamID(int TeamID)
{
	PlayerDataInGame.TeamID = TeamID;
}
#pragma endregion GetterAndSetter

void ATPSPlayerState::AddPersonalScore(int PersonalScore)
{
	PlayerDataInGame.PersonalScore += PersonalScore;
}

void ATPSPlayerState::AddKills(int Kills)
{
	PlayerDataInGame.Kills += Kills;
}

void ATPSPlayerState::AddDeaths(int Deaths)
{
	PlayerDataInGame.Deaths += Deaths;
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
