// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class TPSGAME_API InventoryDealUseItemClass
{
public:
	InventoryDealUseItemClass();
	~InventoryDealUseItemClass();

	static bool DealUseItemByItemID(int ItemID, int ItemQuantity, APlayerController* Instigator);
};
