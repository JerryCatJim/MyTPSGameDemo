// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TPSGameType/SearchSessionResultType.h"
#include "TPSGameInstance.generated.h"

/**
 * 
 */

UCLASS()
class TPSGAME_API UTPSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void TryJoinSession(const FMySearchSessionResult& SessionResult);
	UFUNCTION(BlueprintCallable)
	void TryQuickMatch(int32 MaxSearchResults, bool UseSteam);
	UFUNCTION(BlueprintCallable)
	void TryCreateSession(int32 NumPublicConnections, FString MatchType, FString PathToLobby, bool UseSteam);
	UFUNCTION(BlueprintCallable)
	void TryFindSession(int32 MaxSearchResults, bool UseSteam);
protected:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void QuickMatch_BP(int32 MaxSearchResults, bool UseSteam);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void CreateSession_BP(int32 NumPublicConnections, const FString& PathToLobby, bool UseSteam);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void FindSession_BP(int32 MaxSearchResults, bool UseSteam);

public:
	UPROPERTY(BlueprintReadWrite)
	FString TravelPath;
	
private:
	// The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
};
