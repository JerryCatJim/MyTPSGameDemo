// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/BaseWeapon/ProjectileWeapon.h"
#include "Weapon/Projectile/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"

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
	if(MuzzleSocket)
	{
		FTransform SocketTransform = MuzzleSocket->GetSocketTransform(GetWeaponMeshComp());
		//枪口 指向 准星瞄准方向的终点 的向量
		FVector ToTarget = GetCurrentAimingPoint() - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		if(ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = this;
			SpawnParameters.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if(World)
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					SocketTransform.GetLocation(),
					TargetRotation,
					SpawnParameters
				);
			}
		}
	}
}
