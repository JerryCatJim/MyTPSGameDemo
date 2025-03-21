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
	void OnMatchEnded(int NewWinnerID, ETeam NewWinningTeam);

	UFUNCTION(NetMulticast, Reliable)
	void Multi_OnMatchEnd(int NewWinnerID, ETeam NewWinningTeam, int WinnerScore, bool IsTeamMatchMode);

	UFUNCTION(BlueprintCallable)
	void NewPlayerJoined(APlayerController* Controller);

	UFUNCTION(BlueprintCallable)
	void PlayerLeaveGame(AController* Controller);

	UFUNCTION(BlueprintCallable)
	void AddPlayerPoint(int PlayerID, int AddScore);
	UFUNCTION(BlueprintCallable)
	void AddTeamPoint(ETeam Team, int AddScore, int PlayerID);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void RefreshPlayerScoreBoardUI(AController* Controller, bool RemovePlayer);

	UFUNCTION(BlueprintCallable)
	void UpdateScoreBoard(int PlayerID, ETeam Team, int PlayerScore);

#pragma region GetterAndSetter
	int GetWinnerID(UPARAM(ref) int& WinnerScore);
	ETeam GetWinningTeam();

	int GetTeamScore(ETeam Team);
	
	UFUNCTION(BlueprintCallable)
	float GetWinThreshold() const { return WinThreshold; }
	UFUNCTION(BlueprintCallable)
	float GetIsTeamMatchMode() const { return bIsTeamMatchMode; }
	UFUNCTION(BlueprintCallable)
	float GetRespawnCount() const { return RespawnCount; }
#pragma endregion GetterAndSetter
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void SortPlayerRank_Stable(UPARAM(ref)TArray<FPlayerDataInGame>& PlayerDataArray, bool HighToLow = true);
	UFUNCTION(BlueprintCallable)
	void SortPlayerScoreRank(int PlayerIdToIgnore = -1, bool RemovePlayer = false);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_AnnounceKill(ATPSPlayerState* Killer, const FString& Victim, ETeam VictimTeam);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_AnnounceKill(ATPSPlayerState* Killer, const FString& Victim, ETeam VictimTeam);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnnounceGainKill(ATPSPlayerState* Killer, const FString& Victim, ETeam VictimTeam);

	UFUNCTION(Server, Reliable)
	void Server_GainScore(ATPSPlayerState* Gainer, EGainScoreType GainScoreReason, const FString& VictimName, ETeam VictimTeam);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_GainScore(ATPSPlayerState* Gainer, EGainScoreType GainScoreReason, const FString& VictimName, ETeam VictimTeam);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void AnnounceGainScore(ATPSPlayerState* Gainer, EGainScoreType GainScoreReason, const FString& VictimName, ETeam VictimTeam);

	UFUNCTION(NetMulticast, Reliable)
	void Multi_CallRefreshScoreUI(const TArray<FPlayerDataInGame>& PlayerDataArray);

	UFUNCTION()
	void OnRep_RedTeamScore();
	UFUNCTION()
	void OnRep_BlueTeamScore();
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnScoreUpdated OnScoreUpdated;

	//这里的有些变量设置到protected里用Getter和Setter可能更好，暂时先不改了
	UPROPERTY(BlueprintReadWrite)
	TMap<int, int> PlayerScoreBoard;

	UPROPERTY(BlueprintReadWrite)
	TMap<int, int> BlueTeamScoreBoard;
	UPROPERTY(BlueprintReadWrite)
	TMap<int, int> RedTeamScoreBoard;
	
	UPROPERTY(BlueprintReadWrite)//, Replicated)
	TArray<FPlayerDataInGame> PlayerDataInGameArray;

	UPROPERTY(BlueprintReadWrite)//, Replicated)
	int WinnerID;

	UPROPERTY(BlueprintReadWrite)//, Replicated)
	ETeam WinningTeam;

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
	int RedTeamScore = 0;
	UPROPERTY(ReplicatedUsing=OnRep_BlueTeamScore)
	int BlueTeamScore = 0;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int WinThreshold;

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsTeamMatchMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	float RespawnCount = -1;
};
