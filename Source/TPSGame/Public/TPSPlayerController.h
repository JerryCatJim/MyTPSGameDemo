// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "Component/InventoryComponent.h"

#include "TPSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATPSPlayerController();
	
public:
	//背包组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UInventoryComponent* InventoryComponent;
};
