// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TPSPlayerState.h"
#include "TPSGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreUpdated, const TArray<FPlayerDataInGame>&, PlayerDataArray);
/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()  //在GameMode中绑定，所以在Server触发
	void OnMatchEnded(int NewWinnerID, int NewWinningTeamID);

	UFUNCTION(NetMulticast, Reliable)
	void Multi_OnMatchEnd(int NewWinnerID, int NewWinningTeamID, int WinnerScore, bool IsTeamMatchMode);

	UFUNCTION(BlueprintCallable)
	void NewPlayerJoined(APlayerController* Controller);

	UFUNCTION(BlueprintCallable)
	void PlayerLeaveGame(AController* Controller);

	UFUNCTION(BlueprintCallable)
	void AddPlayerPoint(int PlayerID, int PlayerScore){PlayerScoreBoard.Add(PlayerID, PlayerScore);}
	UFUNCTION(BlueprintCallable)
	void AddTeamPoint(int TeamID, int TeamScore){TeamScoreBoard.Add(TeamID, TeamScore);}

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void RefreshPlayerScoreBoardUI(AController* Controller, bool RemovePlayer);

	UFUNCTION(BlueprintCallable)
	void UpdateScoreBoard(int PlayerID, int TeamID, int PlayerScore);
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void SortPlayerRank_Stable(UPARAM(ref)TArray<FPlayerDataInGame>& PlayerDataArray, bool HighToLow = true);
	UFUNCTION(BlueprintCallable)
	void SortPlayerScoreRank(int PlayerIdToIgnore = -1, bool RemovePlayer = false);
	
	UFUNCTION(Server, Reliable)
	void Server_AnnounceKill(APlayerState* Killer, const FString& Victim);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_AnnounceKill(APlayerState* Killer, const FString& Victim);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnnounceGainKill(APlayerState* Killer, const FString& Victim);

	UFUNCTION(Server, Reliable)
	void Server_GainScore(APlayerState* Gainer, EGainScoreType GainScoreReason, const FString& RightName);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_GainScore(APlayerState* Gainer, EGainScoreType GainScoreReason, const FString& RightName);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnnounceGainScore(APlayerState* Gainer, EGainScoreType GainScoreReason, const FString& RightName);

	UFUNCTION(NetMulticast, Reliable)
	void Multi_CallRefreshScoreUI(const TArray<FPlayerDataInGame>& PlayerDataArray);

	UFUNCTION()
	void OnRep_RedTeamScore();
	UFUNCTION()
	void OnRep_BlueTeamScore();
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnScoreUpdated OnScoreUpdated;
	
	UPROPERTY(BlueprintReadWrite)
	TMap<int, int> PlayerScoreBoard;

	UPROPERTY(BlueprintReadWrite)
	TMap<int, int> TeamScoreBoard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int WinThreshold;

	UPROPERTY(BlueprintReadWrite, Replicated)
	bool bIsTeamMatchMode;

	UPROPERTY(BlueprintReadWrite)//, Replicated)
	TArray<FPlayerDataInGame> PlayerDataInGameArray;

	UPROPERTY(BlueprintReadWrite)//, Replicated)
	int WinnerID;

	UPROPERTY(BlueprintReadWrite)//, Replicated)
	int WinningTeamID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int KillPlayerEnemyScore = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int KillAIEnemyScore = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PickUpPropScore = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int HealTeammateScore = 1;

	UPROPERTY()
	TArray<ATPSPlayerState*> RedTeam;

	UPROPERTY()
	TArray<ATPSPlayerState*> BlueTeam;

	UPROPERTY(ReplicatedUsing=OnRep_RedTeamScore)
	float RedTeamScore;
	UPROPERTY(ReplicatedUsing=OnRep_BlueTeamScore)
	float BlueTeamScore;
	
};
