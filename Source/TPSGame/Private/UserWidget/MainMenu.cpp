// Fill out your copyright notice in the Description page of Project Settings.


#include "UserWidget/MainMenu.h"
#include "MultiplayerSessionsSubsystem.h"

void UMainMenu::TryDestroySession()
{
	UGameInstance* GameInstance = GetGameInstance();
	if(GameInstance)
	{
		MultiplayerSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionSubsystem)
		{
			MultiplayerSessionSubsystem->DestroySession();
		}
	}
}
