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

	virtual void Destroyed() override;
	
protected:
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;
	
	virtual void DealFire() override;

private:
	void DrawMovementTrajectory();
	void ClearMovementTrajectory();
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //武器是否显示抛物线轨迹
	bool CanShowMovementTrajectory = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //是否只在瞄准时显示轨迹，false的话为常驻显示(包括换弹时)
	bool OnlyDrawTrajectoryWhenAiming = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //是否使轨迹保持不动(激光瞄准可能晃动)
	bool KeepTrajectoryStable = true;
	
protected:
	UPROPERTY(EditAnywhere, Category= "Weapon")
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(VisibleAnywhere, Category=Component)
	class USplineComponent* SplineComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)
	float ProjectileGravityZScale = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)
	float ProjectileInitialSpeed = 1500.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)
	float ProjectileMaxSpeed = 1500.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //是否使用武器设置的数值覆盖其发射的子弹类的ProjectileMovementComponent的默认数值
	bool OverrideProjectileDefaultData = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //如果子弹是AOE伤害，需传入内圈和外圈范围，从内圈范围向外会衰减伤害直到外圈范围
	float ProjectileInnerRadius = 150;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //如果子弹是AOE伤害，需传入内圈和外圈范围，从内圈范围向外会衰减伤害直到外圈范围
	float ProjectileOuterRadius = 300;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //绘制预测轨迹应用的重力系数，可设置得与子弹实际系数不同
	float TrajectoryGravityZScale = 1.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //子弹生成时相对枪口上扬的角度
	float UpAngelOffset = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile, meta=(ClampMin = 1.f, ClampMax = 999.f))  //生成抛物线离散点的频率
	float DrawFrequency = 99.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile, meta=(ClampMin = 0.1f, ClampMax = 5.f))  //总共绘制模拟子弹飞行设定秒数的抛物线
	float DrawTrajectoryTime = 1.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile)  //是否只在本地绘制而不同步到服务器
	bool DrawTrajectoryLocally = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=WeaponProjectile, meta=(ClampMin = 0))  //绘制抛物线的间隔(隔几段绘制一次)
	int DrawTrajectoryJumpNum = 0;
	
private:
	UPROPERTY()
	const USkeletalMeshSocket* MuzzleSocket;

	UPROPERTY(EditDefaultsOnly, Category=ProjectileTrajectory)
	TSubclassOf<AActor> TrajectoryTargetPointClass;
	UPROPERTY()
	AActor* TrajectoryTargetPointActor = nullptr;

	UPROPERTY(EditDefaultsOnly, Category=ProjectileTrajectory)
	UStaticMesh* TrajectoryLineMesh;
	UPROPERTY()
	TArray<class USplineMeshComponent*> TrajectoryLineArray;

	UPROPERTY(EditDefaultsOnly, Category=ProjectileTrajectory)
	float TrajectoryLineScale = 1.0f;
};
