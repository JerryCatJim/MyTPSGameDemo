// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/BaseWeapon/ProjectileWeapon.h"
#include "Weapon/Projectile/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"  //射线检测显示颜色

AProjectileWeapon::AProjectileWeapon()
{
	WeaponBulletType = EWeaponBulletType::Projectile;
}

void AProjectileWeapon::DealFire()
{
	//Super::DealFire();
	//与射线检测的即时命中武器不同，发射器武器完全重写发射逻辑

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleSocket = GetWeaponMeshComp()->GetSocketByName(MuzzleSocketName);

	//没有MuzzleSocket的武器无法发射子弹
	if(!MuzzleSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
					FString::Printf(TEXT("当前武器未设置MuzzleFlash位置!")));
		return;
	}
	
	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(GetWeaponMeshComp());
	//枪口 指向 准星瞄准方向的终点 的向量
	FVector ToTarget = GetCurrentAimingPoint() - SocketTransform.GetLocation();
	FRotator TargetRotation = ToTarget.Rotation();
	if(ProjectileClass && InstigatorPawn)
	{
		UWorld* World = GetWorld();
		if(World)
		{
			FTransform SpawnTransform = FTransform(TargetRotation,SocketTransform.GetLocation());
			AProjectile* SpawnProjectile = World->SpawnActorDeferred<AProjectile>(
				ProjectileClass,
				SpawnTransform,
				this,
				InstigatorPawn
			);
			if(SpawnProjectile)
			{
				if(OverrideProjectileDefaultData)
				{
					SpawnProjectile->GravityZScale = ProjectileGravityZScale;
					SpawnProjectile->InitialSpeed = ProjectileInitialSpeed;
					SpawnProjectile->MaxSpeed = ProjectileMaxSpeed;
					SpawnProjectile->WasOverrideFromWeapon = true;
				}
				SpawnProjectile->FinishSpawning(SpawnTransform);
			}
		}
	}

	//DrawDebugLine(GetWorld(), SocketTransform.GetLocation(), GetCurrentAimingPoint(), FColor::Red, false, 1.0f, 0,1.0f);
}
