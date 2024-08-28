// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameMode.h"
#include "SCharacter.h"
#include "TPSGameState.h"

void ATPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	MyGameState = GetGameState<ATPSGameState>();
}

void ATPSGameMode::SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC)
{
	Super::SwapPlayerControllers(OldPC, NewPC);

	//ServerTravel时触发OnSwap而不是PostLogin，写在蓝图中的OnSwapPlayerControllers也可以
	ATPSGameState* GS = GetGameState<ATPSGameState>();
	if(GS)
	{
		GS->NewPlayerJoined(NewPC);
	}
}

void ATPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ATPSGameState* GS = GetGameState<ATPSGameState>();
	if(GS)
	{
		GS->NewPlayerJoined(NewPlayer);
		GS->RefreshPlayerScoreBoardUI(NewPlayer, true);
	}
}

void ATPSGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ATPSGameState* GS = GetGameState<ATPSGameState>();
	if(GS)
	{
		GS->RefreshPlayerScoreBoardUI(Exiting, false);
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

void ATPSGameMode::PlayerLeaveGame()
{
	
}

bool ATPSGameMode::ReadyToEndMatch_Implementation()
{
	//const bool RetVal = Super::ReadyToEndMatch_Implementation();

	if(!MyGameState)
	{
		MyGameState = GetGameState<ATPSGameState>();
	}
	if(MyGameState)
	{
		TMap<int, int> TempMap = bIsTeamMatchMode ? MyGameState->TeamScoreBoard : MyGameState->PlayerScoreBoard;
		TArray<int> TempKeysArray;
		TempMap.GetKeys(TempKeysArray);
		for(auto& TempID : TempKeysArray)
		{
			if(TempMap.Find(TempID) && TempMap.FindRef(TempID) >= WinThreshold)
			{
				WinnerID = bIsTeamMatchMode ? -1 : TempID;
				WinningTeamID = bIsTeamMatchMode ? TempID : -1;
				OnMatchEnd.Broadcast(WinnerID, WinningTeamID);

				return true;
			}
		}
		return false;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TPSGameMode : %s中的TPSGameState转换失败"), *FString(__FUNCTION__));
		return false;
	}
}
