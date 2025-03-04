// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/TPSHUD.h"

#include "SCharacter.h"
#include "UserWidget/TPSCrossHair.h"

ATPSHUD::ATPSHUD()
{
	//从C++中获取蓝图类
	//const FString WidgetClassLoadPath = FString(TEXT("'/Game/UI/GameUI/EndGameScreen.EndGameScreen_C'"));//蓝图一定要加_C这个后缀名
	//EndGameScreenClass = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
}

void ATPSHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ATPSHUD::ResetCrossHairWidget(APlayerController* PlayerController)
{
	RemoveCrossHairWidget();
	if(PlayerController)
	{
		if(PlayerController->GetPawn())
		{
			ASCharacter* PlayerCharacter = Cast<ASCharacter>(PlayerController->GetPawn());
			if(PlayerCharacter && PlayerCharacter->GetCurrentWeapon() && PlayerCharacter->GetCurrentWeapon()->CrossHairClass)
			{
				CrossHairView = CreateWidget<UTPSCrossHair>(PlayerController, PlayerCharacter->GetCurrentWeapon()->CrossHairClass);
				CrossHairView->AddToViewport();
				AddToWidgetList(CrossHairView);
			}
		}
	}
}

void ATPSHUD::RemoveCrossHairWidget()
{
	if(CrossHairView)
	{
		CrossHairView->RemoveFromParent();
		WidgetsList.Remove(CrossHairView);
	}
}

void ATPSHUD::SetCrossHairVisibility(bool IsVisible)
{
	if(!IsValid(CrossHairView)) return;
	if(IsVisible)
	{
		CrossHairView->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		CrossHairView->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ATPSHUD::AddToWidgetList(UUserWidget* WidgetToRecord)
{
	WidgetsList.Emplace(WidgetToRecord);
}

void ATPSHUD::Destroyed()
{
	Super::Destroyed();

	for(UUserWidget* WidgetToRemove : WidgetsList)
	{
		if(IsValid(WidgetToRemove))
		{
			WidgetToRemove->RemoveFromParent();
		}
	}
}


