// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BaseWeapon/ProjectileWeapon.h"
#include "ThrowGrenade.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AThrowGrenade : public AProjectileWeapon  //扔手雷本质和发射器武器是一样的
{
	GENERATED_BODY()

public:
	AThrowGrenade();
	
};
