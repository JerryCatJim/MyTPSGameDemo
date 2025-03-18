// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TPSGameMode.h"
#include "SCharacter.h"
#include "TPSGameState.h"
#include "TPSPlayerController.h"

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
	
	SetHasGameBegun(true); //后续可以做个倒计时后开始游戏，将此处移到那里
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

	APawn* OldPawn = PlayerController->GetPawn();
	
	//如果死亡时切换角色，希望复活后再变为新角色
	ATPSPlayerController* TPC = Cast<ATPSPlayerController>(PlayerController);
	if(TPC)
	{
		//AActor* StartSpot = StartSpot = TPC->StartSpot.Get();
		AActor* StartSpot = FindPlayerStart(TPC);
		// If a start spot wasn't found,
		if (StartSpot == nullptr)
		{
			// Check for a previously assigned spot
			if (TPC->StartSpot != nullptr)
			{
				StartSpot = TPC->StartSpot.Get();
				UE_LOG(LogGameMode, Warning, TEXT("RestartPlayer: Player start not found, using last start spot"));
			}	
		}
		const FTransform NewPawnPos = StartSpot ? FTransform(StartSpot->GetActorLocation()) : FTransform();
	
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = PlayerController;  //要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空
		SpawnParams.Instigator = GetInstigator();
		TWeakObjectPtr<ASCharacter> NewCharacter =  GetWorld()->SpawnActor<ASCharacter>(
			TPC->GetNewPawnClassToPossess(),
			NewPawnPos,
			SpawnParams
		);
		TPC->Possess(NewCharacter.Get());
	}
	
	RestartPlayer(PlayerController);
	
	if(OldPawn)
	{
		OldPawn->Destroy();
	}
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
			int WinnerScore = 0;
			WinnerID = MyGameState->GetWinnerID(WinnerScore);
			if(WinnerID != -1)
			{
				WinningTeam = ETeam::ET_NoTeam;
				OnMatchEnd.Broadcast(WinnerID, WinningTeam);
				return true;
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
