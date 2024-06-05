// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Component/InventoryComponent.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int TeamID = -1;
};

UCLASS()
class TPSGAME_API ATPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const override;

	virtual void BeginPlay() override;

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

	UFUNCTION(BlueprintCallable)
	int GetTeamID() const;
	UFUNCTION(BlueprintCallable)
	void SetTeamID(int TeamID);
#pragma endregion GetterAndSetter
	UFUNCTION(BlueprintCallable)
	void AddPersonalScore(int PersonalScore);
	UFUNCTION(BlueprintCallable)
	void AddKills(int Kills);
	UFUNCTION(BlueprintCallable)
	void AddDeaths(int Deaths);

protected:
	virtual void CopyProperties(APlayerState* PlayerState) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	TArray<FMyItem> ItemList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	FPlayerDataInGame PlayerDataInGame;
};
