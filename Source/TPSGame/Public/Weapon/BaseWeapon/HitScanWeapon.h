// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BaseWeapon/SWeapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AHitScanWeapon : public ASWeapon
{
	GENERATED_BODY()

public:
	AHitScanWeapon();

protected:
	virtual void DealFire() override;
};
