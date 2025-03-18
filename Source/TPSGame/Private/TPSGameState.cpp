// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameState.h"

#include "SCharacter.h"
#include "GameMode/TPSGameMode.h"
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
				//GameMode只在服务器有，若GameState想实时同步这些变量的修改，可以在GameMode中设置一些委托绑定变量，以便在GameState中绑定，然后Replicate到客户端的GameState中以便读取
				WinThreshold = MyGameMode->GetWinThreshold();
				bIsTeamMatchMode = MyGameMode->GetIsTeamMatchMode();
				RespawnCount = MyGameMode->GetRespawnCount();

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

void ATPSGameState::AddPlayerPoint(int PlayerID, int AddScore)
{
	if(!PlayerScoreBoard.Contains(PlayerID))
	{
		PlayerScoreBoard.Add(PlayerID, AddScore);
	}
	else
	{
		PlayerScoreBoard.Add(PlayerID, PlayerScoreBoard.FindRef(PlayerID)+ AddScore);
	}
}

void ATPSGameState::AddTeamPoint(ETeam Team, int TeamScore, int PlayerID)
{
	switch(Team)
	{
	case ETeam::ET_BlueTeam:
		BlueTeamScore += TeamScore;
		BlueTeamScoreBoard.Add(PlayerID, TeamScore);
		break;
	case ETeam::ET_RedTeam:
		RedTeamScore += TeamScore;
		RedTeamScoreBoard.Add(PlayerID, TeamScore);
		break;
	case ETeam::ET_NoTeam:
	default:
		break;
	}
}

int ATPSGameState::GetTeamScore(ETeam Team)
{
	switch(Team)
	{
	case ETeam::ET_BlueTeam:
		return BlueTeamScore;
	case ETeam::ET_RedTeam:
		return RedTeamScore;
	case ETeam::ET_NoTeam:
	default:
		return -1;
	}
}

void ATPSGameState::UpdateScoreBoard(int PlayerID, ETeam Team, int AddScore)
{
	if(bIsTeamMatchMode)
	{
		AddTeamPoint(Team, AddScore, PlayerID);
	}
	else
	{
		AddPlayerPoint(PlayerID, AddScore);
	}

	//刷新一下排行榜
	if(HasAuthority())
	{
		SortPlayerScoreRank();
		Multi_CallRefreshScoreUI(PlayerDataInGameArray);
	}
}

int ATPSGameState::GetWinnerID(int& WinnerScore)
{
	TArray<int> TempKeysArray;
	PlayerScoreBoard.GetKeys(TempKeysArray);
	for(auto& TempID : TempKeysArray)
	{
		if(PlayerScoreBoard.Contains(TempID) && PlayerScoreBoard.FindRef(TempID) >= WinThreshold)
		{
			WinnerID = TempID;
			WinnerScore = PlayerScoreBoard.FindRef(TempID);
			WinningTeam = ETeam::ET_NoTeam;
			return WinnerID;
		}
	}
	WinnerScore = 0;
	return -1;
}

ETeam ATPSGameState::GetWinningTeam()
{
	if(BlueTeamScore >= WinThreshold)
	{
		return ETeam::ET_BlueTeam;
	}
	else if(RedTeamScore >= WinThreshold)
	{
		return ETeam::ET_RedTeam;
	}
	else
	{
		return ETeam::ET_NoTeam;
	}
}

void ATPSGameState::OnMatchEnded(int NewWinnerID, ETeam NewWinningTeam)
{
	WinnerID = NewWinnerID;
	WinningTeam = NewWinningTeam;

	const int ShowScore = bIsTeamMatchMode ? GetTeamScore(NewWinningTeam) : PlayerScoreBoard.FindRef(WinnerID);

	//Multicast是Reliable立刻触发，可能比属性复制快，所以传入参数而不是等属性复制
	Multi_OnMatchEnd(WinnerID, WinningTeam, ShowScore, bIsTeamMatchMode);
}

void ATPSGameState::Multi_OnMatchEnd_Implementation(int NewWinnerID, ETeam NewWinningTeam, int WinnerScore, bool IsTeamMatchMode)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if(PC && PC->IsLocalController())
	{
		ASCharacter* Player = Cast<ASCharacter>(PC->GetPawn());
		if(Player) Player->OnMatchEnd(NewWinnerID, NewWinningTeam);

		PC->UnPossess();

		ATPSHUD* HUD = Cast<ATPSHUD>(PC->GetHUD());
		if(HUD)
		{
			HUD->ShowEndGameScreen(NewWinnerID, NewWinningTeam, WinnerScore, IsTeamMatchMode);
		}
	}
}

void ATPSGameState::OnRep_RedTeamScore()
{
}

void ATPSGameState::OnRep_BlueTeamScore()
{
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
			if(PS->GetTeam() == ETeam::ET_BlueTeam)
			{
				BlueTeamScoreBoard.Remove(PS->GetPlayerId());
			}
			else if(PS->GetTeam() == ETeam::ET_RedTeam)
			{
				RedTeamScoreBoard.Remove(PS->GetPlayerId());
			}
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
		SortPlayerScoreRank(Controller->PlayerState->GetPlayerId(), RemovePlayer);
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

void ATPSGameState::Server_AnnounceKill_Implementation(ATPSPlayerState* Killer, const FString& Victim, ETeam VictimTeam)
{
	Multi_AnnounceKill(Killer, Victim, VictimTeam);
}

void ATPSGameState::Multi_AnnounceKill_Implementation(ATPSPlayerState* Killer, const FString& Victim, ETeam VictimTeam)
{
	//子类不要重载组播，而是重载组播调用的函数，否则会导致客户端多调用一次事件：
	//因为Server端的子类Multi调用了父类的Multi，此时父类Multi在Server和Client都执行了一次，
	//再走到客户端重写的Multi，又调用了一次父类Multi，此时客户端又执行一次，
	//所以客户端一共执行了两次Multi
	AnnounceGainKill(Killer, Victim, VictimTeam);
}

void ATPSGameState::AnnounceGainKill_Implementation(ATPSPlayerState* Killer, const FString& Victim, ETeam VictimTeam)
{
	//蓝图里Override了
}

void ATPSGameState::Server_GainScore_Implementation(ATPSPlayerState* Gainer, EGainScoreType GainScoreReason,
	const FString& VictimName, ETeam VictimTeam)
{
	Multi_GainScore(Gainer, GainScoreReason, VictimName, VictimTeam);
}

void ATPSGameState::Multi_GainScore_Implementation(ATPSPlayerState* Gainer, EGainScoreType GainScoreReason,
	const FString& VictimName, ETeam VictimTeam)
{
	AnnounceGainScore(Gainer, GainScoreReason, VictimName, VictimTeam);
}

void ATPSGameState::AnnounceGainScore_Implementation(ATPSPlayerState* Gainer, EGainScoreType GainScoreReason, const FString& VictimName, ETeam VictimTeam)
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
	DOREPLIFETIME(ATPSGameState, RedTeamScore);
	DOREPLIFETIME(ATPSGameState, BlueTeamScore);
	DOREPLIFETIME(ATPSGameState, RespawnCount);
}
