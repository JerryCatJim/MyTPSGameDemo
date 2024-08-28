// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerState.h"
#include "SCharacter.h"
#include "TPSGameState.h"

#include "Net/UnrealNetwork.h"

void ATPSPlayerState::Reset()
{
	Super::Reset();
}

void ATPSPlayerState::BeginPlay()
{
	Super::BeginPlay();
	TryGetGameState();
}

bool ATPSPlayerState::CheckCanGainKillScore(int VictimID, int VictimTeamID)
{
	return VictimID != GetPlayerId() &&
		CurGameState &&
			(!CurGameState->bIsTeamMatchMode ||
				(GetTeamID() != VictimID && CurGameState->bIsTeamMatchMode));
}

void ATPSPlayerState::TryGetGameState()
{
	UWorld* World = GetWorld();
	if(World)
	{
		GetWorldTimerManager().SetTimer(
			FGetGameStateHandle,
			this,
			&ATPSPlayerState::LoopSetGameState,  //用匿名函数会闪退(?)
			0.2,
			true
			);
	}
}

void ATPSPlayerState::LoopSetGameState()
{
	CurGameState = Cast<ATPSGameState>(GetWorld()->GetGameState());
	if(CurGameState)
	{
		GetWorldTimerManager().ClearTimer(FGetGameStateHandle);
	}
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

void ATPSPlayerState::GainPickUpScore_Implementation(const FString& PickUpName)
{
	OnGainScore.Broadcast(this, EGainScoreType::PickUpScoreProp, PickUpName);
}

void ATPSPlayerState::GainKill_Implementation(const FString& VictimName, int VictimID, int VictimTeamID)
{
	if(CheckCanGainKillScore(VictimID, VictimTeamID))
	{
		//自尽或者杀了队友都不算数
		AddKills(1);
		OnGainScore.Broadcast(this, KillPlayerEnemy, VictimName);
	}
	//触发TPSGameState里的Server_AnnounceKill
	OnKillOtherPlayers.Broadcast(this, VictimName);
}

void ATPSPlayerState::PlayerStateGainScore(int NewScore)
{
	AddPersonalScore(NewScore);
	UWorld* World = GetWorld();
	if(World)
	{
		ATPSGameState* GS = Cast<ATPSGameState>(World->GetGameState());
		if(GS)
		{
			GS->UpdateScoreBoard(GetPlayerId(), GetTeamID(), GetPersonalScore());
		}
	}
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

void ATPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSPlayerState, PlayerDataInGame);
}