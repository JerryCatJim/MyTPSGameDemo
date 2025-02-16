// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BaseWeapon/SWeapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AProjectileWeapon : public ASWeapon
{
	GENERATED_BODY()

public:
	AProjectileWeapon();
	
protected:
	virtual void DealFire() override;

public:

protected:
	UPROPERTY(EditAnywhere, Category= "Weapon")
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)
	float ProjectileGravityZScale = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)
	float ProjectileInitialSpeed = 50.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)
	float ProjectileMaxSpeed = 50.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //是否使用武器设置的数值覆盖其发射的子弹类的ProjectileMovementComponent的默认数值
	bool OverrideProjectileDefaultData = false;
};
