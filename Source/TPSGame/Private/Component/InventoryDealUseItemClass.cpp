// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InventoryDealUseItemClass.h"

#include "Component/InventoryComponent.h"
#include "Component/SHealthComponent.h"

InventoryDealUseItemClass::InventoryDealUseItemClass()
{
}

InventoryDealUseItemClass::~InventoryDealUseItemClass()
{
}

bool InventoryDealUseItemClass::DealUseItemByItemID(int ItemID, int ItemQuantity, APlayerController* Instigator)
{
	if(ItemID == 1)
	{
		if(Instigator && Instigator->GetPawn())
		{
			USHealthComponent* UC = Cast<USHealthComponent>(Instigator->GetPawn()->GetComponentByClass(USHealthComponent::StaticClass()));
			if(UC)
			{
				UC->Heal(100, Instigator, Instigator->GetPawn());
				return true;
			}
			return false;
		}
		return false;
	}
	else
	{
		return false;
	}
}
