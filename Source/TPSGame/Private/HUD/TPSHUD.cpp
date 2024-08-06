// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/TPSHUD.h"

#include "SCharacter.h"
#include "UserWidget/TPSCrossHair.h"

void ATPSHUD::ResetCrossHairWidget(APlayerController* PlayerController)
{
	RemoveCrossHairWidget();
	if(PlayerController)
	{
		if(PlayerController->GetPawn())
		{
			const ASCharacter* PlayerCharacter = Cast<ASCharacter>(PlayerController->GetPawn());
			if(PlayerCharacter && PlayerCharacter->CurrentWeapon && PlayerCharacter->CurrentWeapon->CrossHairClass)
			{
				CrossHairView = CreateWidget<UTPSCrossHair>(PlayerController, PlayerCharacter->CurrentWeapon->CrossHairClass);
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
