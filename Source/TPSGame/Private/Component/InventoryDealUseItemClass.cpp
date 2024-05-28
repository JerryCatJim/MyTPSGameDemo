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

bool InventoryDealUseItemClass::DealUseItemByItemID(int ItemID, int ItemQuantity, APawn* Instigator)
{
	if(ItemID == 1)
	{
		USHealthComponent* UC = Cast<USHealthComponent>(Instigator->GetComponentByClass(USHealthComponent::StaticClass()));
		UC->Heal(100, Instigator->GetController(), Instigator);
		return true;
	}
	else
	{
		return false;
	}
}
