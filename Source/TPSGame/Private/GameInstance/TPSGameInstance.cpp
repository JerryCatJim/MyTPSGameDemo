// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/TPSGameInstance.h"
#include "MultiplayerSessionsSubsystem.h"

void UTPSGameInstance::Init()
{
	Super::Init();

	MultiplayerSessionsSubsystem = GetSubsystem<UMultiplayerSessionsSubsystem>();
}

void UTPSGameInstance::TryJoinSession(const FMySearchSessionResult& SessionResult)
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->JoinSession(SessionResult.OnlineResult);
	}
}

void UTPSGameInstance::TryQuickMatch(int32 MaxSearchResults, bool UseSteam)
{
	if(!UseSteam)
	{
		QuickMatch_BP(MaxSearchResults, UseSteam);
	}
	else
	{
		if (MultiplayerSessionsSubsystem)  //和TryFindSession的区别在于，TPSMenu.cpp中的OnFindSession里的逻辑不同
		{
			MultiplayerSessionsSubsystem->FindSessions(MaxSearchResults);
		}
	}
}

void UTPSGameInstance::TryCreateSession(int32 NumPublicConnections, FString MatchType, FString PathToLobby, bool UseSteam)
{
	TravelPath = PathToLobby;
	if(!UseSteam)
	{
		CreateSession_BP(NumPublicConnections, PathToLobby, UseSteam);
	}
	else
	{
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
		}
	}
}

void UTPSGameInstance::TryFindSession(int32 MaxSearchResults, bool UseSteam)
{
	if(!UseSteam)
	{
		FindSession_BP(MaxSearchResults, UseSteam);
	}
	else
	{
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->FindSessions(MaxSearchResults);
		}
	}
}