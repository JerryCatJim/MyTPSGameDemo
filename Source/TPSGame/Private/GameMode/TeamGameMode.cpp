// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TeamGameMode.h"
#include "TPSGameState.h"
#include "TPSPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "TPSGame/PlayerStart/TeamPlayerStart.h"

ATeamGameMode::ATeamGameMode()
{
	bIsTeamMatchMode = true;
	bCanAttackTeammate = true;
	TeammateDamageRate = 0.2;
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

void ATeamGameMode::RestartPlayer(AController* NewPlayer)
{
	//Super::RestartPlayer(NewPlayer);
	
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	if(!IsValid(NewPlayer->PlayerState))
	{
		SetTeamFailedControllers.AddUnique(NewPlayer);
		UE_LOG(LogGameMode, Warning, TEXT("RestartPlayer: PlayerState获取失败"));
		return;
	}

	//从所有ATeamPlayerStart子类的PlayerStart中找到 所有Team和玩家Team一样的出生点并组成数组，从中随机一个作为此次出生点
	ATPSPlayerState* MyPlayerState = NewPlayer->GetPlayerState<ATPSPlayerState>();
	TArray<AActor*> TeamPlayerStarts;
	TArray<ATeamPlayerStart*> MyTeamPlayerStarts;
	if(MyPlayerState && MyPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), TeamPlayerStarts);
		for(AActor* Start : TeamPlayerStarts)
		{
			ATeamPlayerStart*  MyPlayerStart = Cast<ATeamPlayerStart>(Start);
			if(MyPlayerStart && MyPlayerStart->Team == MyPlayerState->GetTeam())
			{
				MyTeamPlayerStarts.Emplace(MyPlayerStart);
			}
		}
	}
	else
	{
		SetTeamFailedControllers.AddUnique(NewPlayer);
		//UE_LOG(LogGameMode, Warning, TEXT("RestartPlayer: PlayerState的Team为NoTeam，找不到对应颜色队伍的出生点"));
		return;
	}
	
	AActor* StartSpot = nullptr;
	if(MyTeamPlayerStarts.Num() > 0 )
	{
		StartSpot = MyTeamPlayerStarts[FMath::RandRange(0, MyTeamPlayerStarts.Num() - 1)];
	}

	// If a start spot wasn't found,
	if (StartSpot == nullptr)
	{
		// Check for a previously assigned spot
		if (NewPlayer->StartSpot != nullptr)
		{
			StartSpot = NewPlayer->StartSpot.Get();
			UE_LOG(LogGameMode, Warning, TEXT("RestartPlayer: Player start not found, using last start spot"));
		}	
	}

	RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

float ATeamGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	if(Attacker == nullptr || Victim == nullptr) return 0;
	
	ATPSPlayerState* AttackerPS = Attacker->GetPlayerState<ATPSPlayerState>();
	ATPSPlayerState* VictimPS = Victim->GetPlayerState<ATPSPlayerState>();

	if(AttackerPS == nullptr || VictimPS == nullptr) return BaseDamage;
	if(AttackerPS == VictimPS) return BaseDamage;  //自己打自己
	if(AttackerPS->GetTeam() == VictimPS->GetTeam())
	{
		return bCanAttackTeammate ? BaseDamage * TeammateDamageRate : 0.0f;
	}

	return BaseDamage;
}

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
	/*if(MyGameState)
	{
		for(APlayerState* PS : MyGameState->PlayerArray)
		{
			TryAddAndSetTeam(PS);
		}
	}*/
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

	//客户端刚进入游戏时先触发了RestartPlayer然后才触发Login(?)，所以把玩家分配到对应队伍出生点不会成功，
	//记录设置失败的玩家，等Login后调用TryAddAndSetTeam时再重新RestartPlayer一次
	//客户端触发顺序为 : RestartPlayer -> HandleMatchHasStarted -> PostLogin (???)
	for(int i = SetTeamFailedControllers.Num() - 1; i >=0; --i)
	{
		AController* TempController = SetTeamFailedControllers[i];
		SetTeamFailedControllers.RemoveAt(i);
		RestartPlayer(TempController);
	}
}
