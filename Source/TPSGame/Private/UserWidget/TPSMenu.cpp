// Fill out your copyright notice in the Description page of Project Settings.

#include "UserWidget/TPSMenu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "GameInstance/TPSGameInstance.h"

void UTPSMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch, FString LobbyPath)
{
	PathToLobby = LobbyPath;
	FullPathWithOptions = FString::Printf(TEXT("%s?listen"), *LobbyPath);
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	if(!GetParent())
	{
		AddToViewport();
	}
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	MyGameInstance = GetGameInstance<UTPSGameInstance>();
	if (MyGameInstance)
	{
		MultiplayerSessionsSubsystem = MyGameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool UTPSMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}
	if (FindButton)
	{
		FindButton->OnClicked.AddDynamic(this, &ThisClass::FindButtonClicked);
	}

	return true;
}

void UTPSMenu::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	MenuTearDown();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

void UTPSMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(FullPathWithOptions);
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("Failed to create session!"))
			);
		}
		HostButton->SetIsEnabled(true);
	}
}

void UTPSMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	if (MultiplayerSessionsSubsystem == nullptr)
	{
		return;
	}

	if(LastClickedButton == EMenuButtonType::JoinButton)
	{
		for (auto Result : SessionResults)
		{
			FString SettingsValue;
			Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
			if (SettingsValue == MatchType)
			{
				MultiplayerSessionsSubsystem->JoinSession(Result);
				return;
			}
		}
	}
	else if(LastClickedButton == EMenuButtonType::FindButton)
	{
		TArray<FMySearchSessionResult> TempArray;
		if(SessionResults.Num() > 0)
		{
			TempArray.Empty();
			for(auto Result : SessionResults)
			{
				FMySearchSessionResult TempRes;
				TempRes.OnlineResult = Result;
				TempArray.Emplace(TempRes);
			}
			OnFindSessionResultsReceived(TempArray);
		}
	}
	
	if (!bWasSuccessful || SessionResults.Num() == 0)
	{
		JoinButton->SetIsEnabled(true);
	}
}

void UTPSMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem && MyGameInstance)
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			FString Address;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			APlayerController* PlayerController = MyGameInstance->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

void UTPSMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UTPSMenu::OnStartSession(bool bWasSuccessful)
{
}

void UTPSMenu::HostButtonClicked()
{
	LastClickedButton = EMenuButtonType::HostButton;
	HostButton->SetIsEnabled(!bUseSteam);  //如果没点UseSteam则走BP_Instance蓝图内的局域网逻辑，就不用设置按钮不可用了，还得在蓝图里设置回来，麻烦
	if(MyGameInstance)
	{
		MyGameInstance->TryCreateSession(NumPublicConnections, MatchType, PathToLobby, bUseSteam);
	}
}

void UTPSMenu::JoinButtonClicked()
{
	LastClickedButton = EMenuButtonType::JoinButton;
	JoinButton->SetIsEnabled(!bUseSteam);
	if(MyGameInstance)
	{
		MyGameInstance->TryFindSession(10000,bUseSteam);
	}
}

void UTPSMenu::FindButtonClicked()
{
	LastClickedButton = EMenuButtonType::FindButton;
	FindButton->SetIsEnabled(!bUseSteam);
	if(MyGameInstance)
	{
		MyGameInstance->TryFindSession(10000,bUseSteam);
	}
}

void UTPSMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
