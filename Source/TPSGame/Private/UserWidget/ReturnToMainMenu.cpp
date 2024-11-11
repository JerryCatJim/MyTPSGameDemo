// Fill out your copyright notice in the Description page of Project Settings.


#include "UserWidget/ReturnToMainMenu.h"
#include "MultiplayerSessionsSubsystem.h"
#include "TPSPlayerController.h"

#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"

void UReturnToMainMenu::MenuSetup()
{
	if(bCanAddToViewport)
	{
		AddToViewport();
	}
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if(World)
	{
		if(MyPlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			MyPlayerController->SetInputMode(InputModeData);
			MyPlayerController->SetShowMouseCursor(true);
		}
	}
	
	if(ReturnButton && !ReturnButton->OnClicked.IsAlreadyBound(this, &UReturnToMainMenu::ReturnButtonClicked))
	{
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	UGameInstance* GameInstance = GetGameInstance();
	if(GameInstance)
	{
		MultiplayerSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionSubsystem)
		{
			MultiplayerSessionSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	UWorld* World = GetWorld();
	if(World)
	{
		if(MyPlayerController)
		{
			FInputModeGameOnly InputModeData;
			MyPlayerController->SetInputMode(InputModeData);
			MyPlayerController->SetShowMouseCursor(false);
		}
	}
	
	//移除委托绑定函数
	if(ReturnButton && ReturnButton->OnClicked.IsAlreadyBound(this, &UReturnToMainMenu::ReturnButtonClicked))
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}
	if (MultiplayerSessionSubsystem && MultiplayerSessionSubsystem->MultiplayerOnDestroySessionComplete.IsAlreadyBound(this, &UReturnToMainMenu::OnDestroySession))
	{
		MultiplayerSessionSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
	}
	
	RemoveFromParent();
}

void UReturnToMainMenu::ShowOrHideReturnToMainMenu()
{
	bShowWidget = !bShowWidget;
	if(bShowWidget)
	{
		MenuSetup();
	}
	else
	{
		MenuTearDown();
	}
}

bool UReturnToMainMenu::Initialize()
{
	if(!Super::Initialize())
	{
		return false;
	}
	
	if(GetOwningPlayer())
	{
		MyPlayerController = Cast<ATPSPlayerController>(GetOwningPlayer());
	}
	else
	{
		UWorld* World = GetWorld();
		if(World)
		{
			MyPlayerController = Cast<ATPSPlayerController>(World->GetFirstPlayerController());
		}
	}
	
	if(MyPlayerController)
	{
		MyPlayerController->OnPlayerLeaveGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
	}

	if(bShowWidgetAfterCreate)
	{
		MenuSetup();
		bShowWidget = true;
	}
	
	return true;
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
	if(!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}
	
	UWorld* World = GetWorld();
	if(World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if(GameMode)
		{
			GameMode->ReturnToMainMenuHost();
		}
		else
		{
			if(MyPlayerController)
			{
				MyPlayerController->ClientReturnToMainMenuWithTextReason(FText());
				//PlayerController->ClientReturnToMainMenu(TEXT(""));  //此函数UE不推荐使用，建议使用上面那个
			}
		}
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled(false);
	
	UWorld* World = GetWorld();
	if(World)
	{
		ATPSPlayerController* FirstPlayerController = Cast<ATPSPlayerController>(World->GetFirstPlayerController());
		if(FirstPlayerController)
		{
			FirstPlayerController->PlayerLeaveGame();
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	if (MultiplayerSessionSubsystem)
	{
		MultiplayerSessionSubsystem->DestroySession();
	}
}