// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/AdvancedWeapon/ThrowGrenade.h"
#include "Components/SplineComponent.h"

AThrowGrenade::AThrowGrenade()
{
	WeaponType = EWeaponType::ThrowGrenade;
	WeaponBulletType = EWeaponBulletType::Projectile;
	ZoomedFOV = 90.f;
	RateOfShoot = 60.f;
	BulletSpread = 0.f;  //指哪扔哪，不会抖动

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(GetRootComponent());
	
	OnePackageAmmoNum = 1;
	CurrentAmmoNum = OnePackageAmmoNum;
	BackUpAmmoNum = 300;
	OnceReloadAmmoNum = OnePackageAmmoNum;
	
	WeaponName = TEXT("默认手雷");

	bIsFullAutomaticWeapon = false;
	CanOverloadAmmo = false;

	bCanManuallyDiscard = false;

	ProjectileGravityZScale = 0.5f;
	ProjectileInitialSpeed = 1500.f;
	ProjectileMaxSpeed = 1500.f;
	OverrideProjectileDefaultData = true;
	CanShowMovementTrajectory = true;
	UpAngelOffset = 10.f;
	DrawFrequency = 50.f;
	DrawTrajectoryTime = 2.f;
	DrawTrajectoryLocally = true;
}
