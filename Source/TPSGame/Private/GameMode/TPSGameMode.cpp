// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TPSGameMode.h"
#include "SCharacter.h"
#include "TPSGameState.h"

ATPSGameMode::ATPSGameMode()
{
	bIsTeamMatchMode = false;
}

void ATPSGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
}

void ATPSGameMode::SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC)
{
	Super::SwapPlayerControllers(OldPC, NewPC);

	//ServerTravel时触发OnSwap而不是PostLogin，写在蓝图中的OnSwapPlayerControllers也可以
	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
	if(MyGameState)
	{
		MyGameState->NewPlayerJoined(NewPC);
	}
}

void ATPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
	if(MyGameState)
	{
		MyGameState->NewPlayerJoined(NewPlayer);
	}
}

void ATPSGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
	if(MyGameState)
	{
		MyGameState->PlayerLeaveGame(Exiting);
	}
}

void ATPSGameMode::RespawnPlayer(APlayerController* PlayerController)
{
	if(!PlayerController)
	{
		return;
	}

	ASCharacter* PlayerPawn = Cast<ASCharacter>(PlayerController->GetPawn());
	if(PlayerPawn)
	{
		PlayerPawn->Destroy();
		/*if(PlayerPawn->CurrentWeapon)  //SCharacter中的Destroyed()调用了销毁武器
		{
			PlayerPawn->CurrentWeapon->Destroy();
		}*/
	}
	RestartPlayer(PlayerController);
}

float ATPSGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	return BaseDamage;
}

bool ATPSGameMode::ReadyToEndMatch_Implementation()
{
	//const bool RetVal = Super::ReadyToEndMatch_Implementation();
	
	if(MyGameState)
	{
		if(!bIsTeamMatchMode)
		{
			TMap<int, int> TempMap = MyGameState->PlayerScoreBoard;
			TArray<int> TempKeysArray;
			TempMap.GetKeys(TempKeysArray);
			for(auto& TempID : TempKeysArray)
			{
				if(TempMap.Find(TempID) && TempMap.FindRef(TempID) >= WinThreshold)
				{
					WinnerID = TempID;
					WinningTeam = ETeam::ET_NoTeam;
					OnMatchEnd.Broadcast(WinnerID, WinningTeam);
					return true;
				}
			}
		}
		else
		{
			WinnerID = -1;
			WinningTeam = MyGameState->GetWinningTeam();
			if(WinningTeam != ETeam::ET_NoTeam)
			{
				OnMatchEnd.Broadcast(WinnerID, WinningTeam);
				return true;
			}
		}
		return false;
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("TPSGameMode : %s中的TPSGameState转换失败"), *FString(__FUNCTION__));
		return false;
	}
}
