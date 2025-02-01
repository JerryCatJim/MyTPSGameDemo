// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon/ThrowGrenade.h"

AThrowGrenade::AThrowGrenade()
{
	WeaponType = EWeaponType::ThrowGrenade;
	WeaponBulletType = EWeaponBulletType::Projectile;
	ZoomedFOV = 90.f;
	RateOfShoot = 60.f;
	BulletSpread = 0.f;  //指哪扔哪，不会抖动
	
	OnePackageAmmoNum = 1;
	CurrentAmmoNum = OnePackageAmmoNum;
	BackUpAmmoNum = 300;
	OnceReloadAmmoNum = OnePackageAmmoNum;
	
	WeaponName = TEXT("默认手雷");

	bIsFullAutomaticWeapon = false;
	CanOverloadAmmo = false;
}
