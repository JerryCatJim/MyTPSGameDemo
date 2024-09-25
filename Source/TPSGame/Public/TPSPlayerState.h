// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Component/InventoryComponent.h"
#include "TPSGameType/Team.h"
#include "TPSPlayerState.generated.h"

USTRUCT(BlueprintType)
struct FPlayerDataInGame
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PlayerId = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerName = TEXT("");
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PersonalScore = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int RankInGame = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Kills = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Deaths = 0;
};

USTRUCT(BlueprintType)
struct FPlayerInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Level;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsReady = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int TeamID = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PosInRoom = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsHost = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* RankIcon = nullptr;
};

UENUM(BlueprintType)
enum EGainScoreType
{
	KillPlayerEnemy,
	KillAIEnemy,
	PickUpScoreProp,
	HealTeammate
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKillOtherPlayers, APlayerState*, PlayerState, const FString&, Victim);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGainScore, APlayerState*, Gainer, EGainScoreType, GainScoreReason, const FString&, RightName);

UCLASS()
class TPSGAME_API ATPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATPSPlayerState();
	
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

#pragma region GetterAndSetter
	UFUNCTION(BlueprintCallable)
	int GetPersonalScore() const;
	UFUNCTION(BlueprintCallable)
	void SetPersonalScore(int PersonalScore);

	UFUNCTION(BlueprintCallable)
	int GetRankInGame() const;
	UFUNCTION(BlueprintCallable)
	void SetRankInGame(int RankInGame);

	UFUNCTION(BlueprintCallable)
	int GetKills() const;
	UFUNCTION(BlueprintCallable)
	void SetKills(int Kills);

	UFUNCTION(BlueprintCallable)
	int GetDeaths() const;
	UFUNCTION(BlueprintCallable)
	void SetDeaths(int Deaths);

#pragma endregion GetterAndSetter
	UFUNCTION(BlueprintCallable)
	void AddPersonalScore(int AddScore);
	UFUNCTION(BlueprintCallable)
	void AddKills(int Kills);
	UFUNCTION(BlueprintCallable)
	void AddDeaths(int Deaths);

	UFUNCTION(BlueprintCallable)
	void GetKilled(){ AddDeaths(1); }

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void GainPickUpScore(const FString& PickUpName);
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void GainKill(const FString& VictimName, int VictimID, ETeam VictimTeam);
	
	UFUNCTION(BlueprintCallable)
	void PlayerStateGainScore(int NewScore);
	
	UFUNCTION(BlueprintCallable)
	ETeam GetTeam() const { return Team; };
	UFUNCTION(BlueprintCallable)
	void SetTeam(ETeam TeamID);

	UFUNCTION(BlueprintCallable)
	void SetPawnBodyColor();
	
protected:
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CheckCanGainKillScore(int VictimID, ETeam VictimTeam);

	//SetTeam()在TeamGameMode中被调用，所以只在服务端被改变了身体颜色，需在OnRep_Team()中使客户端也设置颜色
	UFUNCTION()
	void OnRep_Team(){ SetPawnBodyColor(); };
	
private:
	void TryGetGameState();
	void LoopSetGameState();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<FMyItem> ItemList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	FPlayerDataInGame PlayerDataInGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPlayerInfo PlayerInfo;
	
	UPROPERTY(BlueprintAssignable)
	FOnKillOtherPlayers OnKillOtherPlayers;

	UPROPERTY(BlueprintAssignable)
	FOnGainScore OnGainScore;
	
protected:
	UPROPERTY()
	class ATPSGameState* CurGameState;

private:
	FTimerHandle FGetGameStateHandle;
	int TryGetGameStateTimes = 0;  //超过5次还没成功就停止计时器

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	ETeam Team = ETeam::ET_NoTeam;
};
