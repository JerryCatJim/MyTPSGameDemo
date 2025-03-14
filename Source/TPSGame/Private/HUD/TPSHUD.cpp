// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/TPSHUD.h"

#include "SCharacter.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Component/SkillComponent.h"
#include "Components/CanvasPanelSlot.h"
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
			if(PlayerCharacter && PlayerCharacter->GetCurrentWeapon())
			{
				if(PlayerCharacter->GetCurrentWeapon()->CrossHairClass)
				{
					CrossHairView = CreateWidget<UTPSCrossHair>(PlayerController, PlayerCharacter->GetCurrentWeapon()->CrossHairClass);
					if(CrossHairView)
					{
						CrossHairView->AddToViewport();
						AddToWidgetList(CrossHairView);
					}
				}
				if(PlayerCharacter->GetCurrentWeapon()->HitFeedbackCrossHairClass)
				{
					HitFeedbackCrossHairView = CreateWidget<UUserWidget>(PlayerController, PlayerCharacter->GetCurrentWeapon()->HitFeedbackCrossHairClass);
					if(HitFeedbackCrossHairView)
					{
						HitFeedbackCrossHairView->AddToViewport();
						AddToWidgetList(HitFeedbackCrossHairView);
						HitFeedbackCrossHairView->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
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
	if(HitFeedbackCrossHairView)
	{
		HitFeedbackCrossHairView->RemoveFromParent();
		WidgetsList.Remove(HitFeedbackCrossHairView);
	}
}

void ATPSHUD::ResetSkillPercentWidget(APlayerController* PlayerController)
{
	RemoveSkillPercentWidget();
	if(PlayerController)
	{
		if(PlayerController->GetPawn())
		{
			ASCharacter* PlayerCharacter = Cast<ASCharacter>(PlayerController->GetPawn());
			if(PlayerCharacter && PlayerCharacter->GetSkillComponent() && PlayerCharacter->GetSkillComponent()->SkillPercentWidgetClass)
			{
				SkillPercentView = CreateWidget<UUserWidget>(PlayerController, PlayerCharacter->GetSkillComponent()->SkillPercentWidgetClass);
				if(SkillPercentView)
				{
					SkillPercentView->AddToViewport();
					AddToWidgetList(SkillPercentView);
				}
			}
		}
	}
}

void ATPSHUD::RemoveSkillPercentWidget()
{
	if(SkillPercentView)
	{
		SkillPercentView->RemoveFromParent();
		WidgetsList.Remove(SkillPercentView);
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


