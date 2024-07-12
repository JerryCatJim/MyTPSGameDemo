// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AProjectileWeapon : public ASWeapon
{
	GENERATED_BODY()

public:

protected:
	virtual void DealFire() override;

public:

protected:
	UPROPERTY(EditAnywhere, Category= "Weapon")
	TSubclassOf<class AProjectile> ProjectileClass;
};
