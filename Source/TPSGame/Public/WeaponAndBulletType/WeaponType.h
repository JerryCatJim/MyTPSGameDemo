#pragma once

UENUM(BlueprintType)
enum EWeaponType
{
	Rifle,
	Pistol,
	RocketLauncher,
	GrenadeLauncher,  //榴弹发射器
	ShotGun,
	MachineGun,
	SniperRifle,
	ThrowGrenade,
	Fist,  //空手，拳头
	Knife,
};

UENUM(BlueprintType)
enum EWeaponEquipType
{
	MainWeapon,
	SecondaryWeapon,
	MeleeWeapon,
	ThrowableWeapon,
};
