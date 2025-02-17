// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BaseWeapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()

public:
	AShotgun();
	
protected:
	//计算武器射击扩散程度
	virtual float GetDynamicBulletSpread() override;
	
	virtual void DealFire() override;

	//不循环N次调用父类的PlayTraceEffect,新写一个函数每次开枪只调用一次，一次性绘制全部轨迹
	UFUNCTION(NetMulticast, Reliable)
	void PlayTraceEffectForShotgun(const TArray<FVector>& TracePointArray);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Weapon_ShotGun)
	int NumberOfPellets = 10;
};
