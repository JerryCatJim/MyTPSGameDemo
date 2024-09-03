// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameState.h"

#include "SCharacter.h"
#include "TPSGameMode.h"
#include "HUD/TPSHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


void ATPSGameState::BeginPlay()
{
	Super::BeginPlay();

	if(HasAuthority())
	{
		if(GetWorld())
		{
			ATPSGameMode* MyGameMode = Cast<ATPSGameMode>(GetWorld()->GetAuthGameMode());
			if(MyGameMode)
			{
				WinThreshold = MyGameMode->WinThreshold;
				bIsTeamMatchMode = MyGameMode->bIsTeamMatchMode;

				MyGameMode->OnMatchEnd.AddDynamic(this, &ATPSGameState::OnMatchEnded);
			}
		}
	}
}

void ATPSGameState::SortPlayerRank_Stable(UPARAM(ref)TArray<FPlayerDataInGame>& PlayerDataArray, bool HighToLow)
{
	PlayerDataArray.StableSort(
		[=](const FPlayerDataInGame& B, const FPlayerDataInGame& A)->bool //第一个参数是Array[Index + 1], 第二个参数才是Array[Index]
		{
			return HighToLow ? A.PersonalScore < B.PersonalScore : A.PersonalScore > B.PersonalScore;
		});

	for(int i = 0; i < PlayerDataArray.Num(); ++i)
	{
		PlayerDataArray[i].RankInGame = i + 1;
	}
}

void ATPSGameState::SortPlayerScoreRank(int PlayerIdToIgnore, bool RemovePlayer)
{
	PlayerDataInGameArray.Empty();
	for(auto PlayerState : PlayerArray)  //退出游戏后有延迟(?)，PlayerArray不会立刻删除该PlayerState，所以手动忽略一下
	{
		if(PlayerState->GetPlayerId() == PlayerIdToIgnore && RemovePlayer)
		{
			//如果是需要从榜上删除的玩家，则不加入数组
		}
		else
		{
			ATPSPlayerState* PS = Cast<ATPSPlayerState>(PlayerState);
			if(PS)
			{
				PS->PlayerDataInGame.PlayerId = PS->GetPlayerId();
				PS->PlayerDataInGame.PlayerName = PS->GetPlayerName();

				PlayerDataInGameArray.Add(PS->PlayerDataInGame);
			}
		}
	}
	SortPlayerRank_Stable(PlayerDataInGameArray, true);
}

void ATPSGameState::UpdateScoreBoard(int PlayerID, int TeamID, int PlayerScore)
{
	if(bIsTeamMatchMode)
	{
		AddTeamPoint(TeamID, PlayerScore);
	}
	else
	{
		AddPlayerPoint(PlayerID, PlayerScore);
	}

	//刷新一下排行榜
	if(HasAuthority())
	{
		SortPlayerScoreRank();
		Multi_CallRefreshScoreUI(PlayerDataInGameArray);
	}
}

void ATPSGameState::OnMatchEnded(int NewWinnerID, int NewWinningTeamID)
{
	WinnerID = NewWinnerID;
	WinningTeamID = NewWinningTeamID;

	const int ShowScore = bIsTeamMatchMode ? TeamScoreBoard.FindRef(WinningTeamID) : PlayerScoreBoard.FindRef(WinnerID);

	//Multicast是Reliable立刻触发，可能比属性复制快，所以传入参数而不是等属性复制
	Multi_OnMatchEnd(WinnerID, WinningTeamID, ShowScore, bIsTeamMatchMode);
}

void ATPSGameState::Multi_OnMatchEnd_Implementation(int NewWinnerID, int NewWinningTeamID, int WinnerScore, bool IsTeamMatchMode)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if(PC)
	{
		ASCharacter* Player = Cast<ASCharacter>(PC->GetPawn());
		if(Player) Player->OnMatchEnd(NewWinnerID, NewWinningTeamID);

		PC->UnPossess();

		ATPSHUD* HUD = Cast<ATPSHUD>(PC->GetHUD());
		if(HUD)
		{
			const int ShowWinnerID = IsTeamMatchMode ? NewWinningTeamID : NewWinnerID;
			HUD->ShowEndGameScreen(ShowWinnerID, WinnerScore, IsTeamMatchMode);
		}
	}
}

void ATPSGameState::NewPlayerJoined(APlayerController* Controller)
{
	if(!Controller) return;
	
	ATPSPlayerState* PS = Cast<ATPSPlayerState>(Controller->PlayerState);
	if(PS)
	{
		PS->OnKillOtherPlayers.AddDynamic(this, &ATPSGameState::Server_AnnounceKill);
		PS->OnGainScore.AddDynamic(this, &ATPSGameState::Server_GainScore);
	}
	RefreshPlayerScoreBoardUI(Controller, false);
}

void ATPSGameState::PlayerLeaveGame(AController* Controller)
{
	if(!Controller) return;
	
	ATPSPlayerState* PS = Cast<ATPSPlayerState>(Controller->PlayerState);
	if(PS)
	{
		PS->OnKillOtherPlayers.RemoveDynamic(this, &ATPSGameState::Server_AnnounceKill);
		PS->OnGainScore.RemoveDynamic(this, &ATPSGameState::Server_GainScore);

		//从数据榜单中移除该玩家分数
		if(bIsTeamMatchMode)
		{
			TeamScoreBoard.Remove(PS->GetPlayerId());
		}
		else
		{
			PlayerScoreBoard.Remove(PS->GetPlayerId());
		}
	}
	RefreshPlayerScoreBoardUI(Controller, true);
}

void ATPSGameState::RefreshPlayerScoreBoardUI_Implementation(AController* Controller, bool RemovePlayer)
{
	if(!Controller || !Controller->PlayerState) return;
	
	if(bIsTeamMatchMode)
	{
		//组队模式的计分板还没做
	}
	else  //重新排列个人竞技计分板
	{
		SortPlayerScoreRank(Controller->PlayerState->GetPlayerId(), RemovePlayer);
	}
	Multi_CallRefreshScoreUI(PlayerDataInGameArray);
}

void ATPSGameState::Multi_CallRefreshScoreUI_Implementation(const TArray<FPlayerDataInGame>& PlayerDataArray)
{
	OnScoreUpdated.Broadcast(PlayerDataArray);
}

void ATPSGameState::Server_AnnounceKill_Implementation(APlayerState* Killer, const FString& Victim)
{
	Multi_AnnounceKill(Killer, Victim);
}

void ATPSGameState::Multi_AnnounceKill_Implementation(APlayerState* Killer, const FString& Victim)
{
	//子类不要重载组播，而是重载组播调用的函数，否则会导致客户端多调用一次事件：
	//因为Server端的子类Multi调用了父类的Multi，此时父类Multi在Server和Client都执行了一次，
	//再走到客户端重写的Multi，又调用了一次父类Multi，此时客户端又执行一次，
	//所以客户端一共执行了两次Multi
	AnnounceGainKill(Killer, Victim);
}

void ATPSGameState::AnnounceGainKill_Implementation(APlayerState* Killer, const FString& Victim)
{
	//蓝图里Override了
}

void ATPSGameState::Server_GainScore_Implementation(APlayerState* Gainer, EGainScoreType GainScoreReason,
	const FString& RightName)
{
	Multi_GainScore(Gainer, GainScoreReason, RightName);
}

void ATPSGameState::Multi_GainScore_Implementation(APlayerState* Gainer, EGainScoreType GainScoreReason,
	const FString& RightName)
{
	AnnounceGainScore(Gainer, GainScoreReason, RightName);
}

void ATPSGameState::AnnounceGainScore_Implementation(APlayerState* Gainer, EGainScoreType GainScoreReason, const FString& RightName)
{
	//蓝图里Override了
}

void ATPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ATPSGameState, PlayerDataInGameArray);
	DOREPLIFETIME(ATPSGameState, bIsTeamMatchMode);
	DOREPLIFETIME(ATPSGameState, WinThreshold);
	//DOREPLIFETIME(ATPSGameState, WinnerID);
	//DOREPLIFETIME(ATPSGameState, WinningTeamID);
}
