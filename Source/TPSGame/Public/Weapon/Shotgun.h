// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AShotgun : public ASWeapon
{
	GENERATED_BODY()

public:
	AShotgun();
	
protected:
	//计算武器射击扩散程度
	virtual float GetDynamicBulletSpread() override;
	
	virtual void DealFire() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Weapon_ShotGun)
	int NumberOfPellets = 10;
};
