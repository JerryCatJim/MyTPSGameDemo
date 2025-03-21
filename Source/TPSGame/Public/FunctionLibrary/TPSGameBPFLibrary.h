// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TPSGameType/SearchSessionResultType.h"
#include "TPSGameBPFLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API UTPSGameBPFLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Screen")
	static bool IsInScreenViewport(const UObject* WorldContextObject, const FVector& WorldPosition);

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MySearchSessionResult")
	static int32 GetPingInMs(const FMySearchSessionResult& Result)
	{
		return Result.OnlineResult.PingInMs;
	}
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MySearchSessionResult")
	static FString GetServerName(const FMySearchSessionResult& Result)
	{
		return Result.OnlineResult.Session.OwningUserName;
	}
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MySearchSessionResult")
	static int32 GetCurrentPlayers(const FMySearchSessionResult& Result)
	{
		return Result.OnlineResult.Session.SessionSettings.NumPublicConnections - Result.OnlineResult.Session.NumOpenPublicConnections;
	}
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="MySearchSessionResult")
	static int32 GetMaxPlayers(const FMySearchSessionResult& Result)
	{
		return Result.OnlineResult.Session.SessionSettings.NumPublicConnections;
	}
	
};
