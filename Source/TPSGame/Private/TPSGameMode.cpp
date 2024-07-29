// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameMode.h"
#include "SCharacter.h"

void ATPSGameMode::RespawnPlayer(APlayerController* PlayerController)
{
	if(!PlayerController)
	{
		return;
	}

	ASCharacter* PlayerPawn = Cast<ASCharacter>(PlayerController->GetPawn());
	if(PlayerPawn)
	{
		PlayerPawn->Destroy();
		/*if(PlayerPawn->CurrentWeapon)  //SCharacter中的Destroyed()调用了销毁武器
		{
			PlayerPawn->CurrentWeapon->Destroy();
		}*/
	}
	RestartPlayer(PlayerController);
}
