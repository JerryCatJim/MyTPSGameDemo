// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/AdvancedWeapon/GrenadeLauncher.h"
#include "Components/SplineComponent.h"

AGrenadeLauncher::AGrenadeLauncher()
{
	WeaponType = EWeaponType::GrenadeLauncher;
	WeaponBulletType = EWeaponBulletType::Projectile;
	ZoomedFOV = 90.f;
	RateOfShoot = 90.f;
	BulletSpread = 0.f;  //指哪扔哪，不会抖动

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	SplineComponent->SetupAttachment(GetRootComponent());
	
	OnePackageAmmoNum = 12;
	CurrentAmmoNum = OnePackageAmmoNum;
	BackUpAmmoNum = 36;
	OnceReloadAmmoNum = 1;
	
	WeaponName = TEXT("默认榴弹发射器");

	bIsFullAutomaticWeapon = true;
	CanOverloadAmmo = false;
	bCanManuallyDiscard = true;

	ProjectileGravityZScale = 0.5f;
	ProjectileInitialSpeed = 1500.f;
	ProjectileMaxSpeed = 1500.f;
	TrajectoryGravityZScale = ProjectileGravityZScale;
	
	OverrideProjectileDefaultData = true;
	CanShowMovementTrajectory = true;
	UpAngelOffset = 8.f;
	DrawFrequency = 50.f;
	DrawTrajectoryTime = 2.f;
	DrawTrajectoryLocally = true;
}
