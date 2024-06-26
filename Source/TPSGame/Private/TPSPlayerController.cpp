// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerController.h"

ATPSPlayerController::ATPSPlayerController()
{
	//背包组件初始化
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}