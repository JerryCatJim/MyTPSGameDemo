// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TPSGameMode.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ATPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	//GameMode存在于服务器，不用加Server关键字
	void RespawnPlayer(APlayerController* PlayerController);
};
