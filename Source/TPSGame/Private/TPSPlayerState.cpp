// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerState.h"
#include "SCharacter.h"
#include "TPSGameState.h"

#include "Net/UnrealNetwork.h"

ATPSPlayerState::ATPSPlayerState()
{
	SetReplicates(true);
}

void ATPSPlayerState::Reset()
{
	Super::Reset();
}

void ATPSPlayerState::BeginPlay()
{
	Super::BeginPlay();
	TryGetGameState();
}

bool ATPSPlayerState::CheckCanGainKillScore(int VictimID, ETeam VictimTeam)
{
	return VictimID != GetPlayerId() &&
		CurGameState &&
			(!CurGameState->GetIsTeamMatchMode() ||
				(GetTeam() != VictimTeam && CurGameState->GetIsTeamMatchMode()));
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
			true,
			0
			);
	}
}

void ATPSPlayerState::LoopSetGameState()
{
	CurGameState = Cast<ATPSGameState>(GetWorld()->GetGameState());
	if(GetWorld() && CurGameState)
	{
		GetWorldTimerManager().ClearTimer(FGetGameStateHandle);
	}
	else
	{
		TryGetGameStateTimes++;
		if(GetWorld() && TryGetGameStateTimes >= 5 )
		{
			//超过5次还没成功就停止计时器
			GetWorldTimerManager().ClearTimer(FGetGameStateHandle);
		}
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
	OnGainScore.Broadcast(this, EGainScoreType::PickUpScoreProp, PickUpName, ETeam::ET_NoTeam);
}

void ATPSPlayerState::GainKill_Implementation(const FString& VictimName, int VictimID, ETeam VictimTeam)
{
	if(CheckCanGainKillScore(VictimID, VictimTeam))
	{
		//自尽或者杀了队友都不算数
		AddKills(1);
		//触发了TPSGameState里的Server_GainScore
		OnGainScore.Broadcast(this, KillPlayerEnemy, VictimName, VictimTeam);
	}
	//触发TPSGameState里的Server_AnnounceKill
	OnKillOtherPlayers.Broadcast(this, VictimName, VictimTeam);
}

void ATPSPlayerState::PlayerStateGainScore(int AddScore)
{
	AddPersonalScore(AddScore);
	UWorld* World = GetWorld();
	if(World)
	{
		ATPSGameState* GS = Cast<ATPSGameState>(World->GetGameState());
		if(GS)
		{
			GS->UpdateScoreBoard(GetPlayerId(), Team, AddScore);
		}
	}
}

void ATPSPlayerState::SetTeam(ETeam NewTeam)
{
	//此函数在TeamGameMode.cpp中调用，所以是服务端调用
	Team = NewTeam;
	SetPawnBodyColor();
}

void ATPSPlayerState::SetPawnBodyColor()
{
	//如果是刚初始化时设置,PlayerPawn还不存在,设置颜色不会成功
	ASCharacter* MyPlayer = GetPawn<ASCharacter>();
	if(MyPlayer)
	{
		MyPlayer->SetBodyColor(Team);
	}
}

void ATPSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ASCharacter* MyPlayer = GetPawn<ASCharacter>();
	if(MyPlayer && MyPlayer->GetInventoryComponent())
	{
		if(MyPlayer->GetInventoryComponent()->ItemList.Num() > 0)
		{
			ItemList = MyPlayer->GetInventoryComponent()->ItemList;
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
	DOREPLIFETIME(ATPSPlayerState, Team);
}