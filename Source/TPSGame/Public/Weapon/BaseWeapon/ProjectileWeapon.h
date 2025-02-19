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
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;
	
	virtual void DealFire() override;

private:
	void DrawMovementTrajectory();
	
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //如果子弹是AOE伤害，需传入内圈和外圈范围，从内圈范围向外会衰减伤害直到外圈范围
	float ProjectileInnerRadius = 150;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //如果子弹是AOE伤害，需传入内圈和外圈范围，从内圈范围向外会衰减伤害直到外圈范围
	float ProjectileOuterRadius = 300;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //武器是否显示抛物线轨迹
	bool CanShowMovementTrajectory = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //子弹生成时相对枪口上扬的角度
	float UpAngelOffset = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile, meta=(ClampMin = 1.f, ClampMax = 999.f))  //生成抛物线离散点的频率
	float DrawFrequency = 99.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile, meta=(ClampMin = 0.1f, ClampMax = 5.f))  //总共绘制模拟子弹飞行设定秒数的抛物线
	float DrawTrajectoryTime = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //是否只在本地绘制而不同步到服务器
	bool DrawTrajectoryLocally = true;
	
private:
	UPROPERTY()
	const USkeletalMeshSocket* MuzzleSocket;

	UPROPERTY()
	TArray<UParticleSystemComponent*> ParticleArray;
};
