// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon/HitScanWeapon.h"
#include "SCharacter.h"
#include "TPSGameState.h"
#include "TPSGameType/CustomCollisionType.h"
#include "TPSGameType/CustomSurfaceType.h"
#include "Kismet/GameplayStatics.h"

AHitScanWeapon::AHitScanWeapon()
{
	WeaponBulletType = EWeaponBulletType::HitScan;
}

void AHitScanWeapon::DealFire()
{
	FVector StartPoint = GetWeaponShootStartPoint();
	FVector EndPoint = GetCurrentAimingPoint();//EyeLocation + (NewShotDirection*WeaponTraceRange);
	ShotTraceEnd = EndPoint;
	
	//碰撞查询
	FCollisionQueryParams QueryParams;
	//忽略武器自身和持有者的碰撞
	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
	QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质
	
	//击中结果
	FHitResult Hit;
	//射线检测
	bool bIsTraceHit;  //是否射线检测命中
	bIsTraceHit = GetWorld()->LineTraceSingleByChannel(Hit, StartPoint, EndPoint, Collision_Weapon, QueryParams);
	if(bIsTraceHit)
	{
		//伤害处理

		//击中物体
		AActor* HitActor = Hit.GetActor();

		//实际伤害
		float ActualDamage = BaseDamage;
			
		//获得表面类型 PhysicalMaterial
		//EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());  //弱引用
		EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(Hit);
		
		//爆头伤害加成
		if(SurfaceType == Surface_FleshVulnerable)
		{
			ActualDamage *= HeadShotBonus;
		}
		
		//应用伤害
		UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, EndPoint-StartPoint, Hit, MyOwner->GetInstigatorController(), this, DamageType);
		//广播命中事件
		if(HitActor)
		{
			bool IsEnemy = true;
			APawn* HitTarget = Cast<APawn>(HitActor);
			if(HitTarget)
			{
				ATPSPlayerState* PS = HitTarget->GetPlayerState<ATPSPlayerState>();
				ATPSPlayerState* MyPS = MyOwner->GetPlayerState<ATPSPlayerState>();
				if(PS && MyPS)
				{
					ATPSGameState* GS = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
					if(GS && GS->GetIsTeamMatchMode())
					{
						IsEnemy = MyPS->GetTeam() != PS->GetTeam();
					}
				}
			}
			//将击中事件广播出去，可用于HitFeedBackCrossHair这个UserWidget播放击中特效等功能
			Multi_WeaponHitTargetBroadcast(IsEnemy,SurfaceType == Surface_FleshVulnerable);
		}
			
		//若击中目标则将轨迹结束点设置为击中点
		ShotTraceEnd = Hit.ImpactPoint;
		HitSurfaceType = SurfaceType;
		
		PlayImpactEffectsAndSounds(HitSurfaceType, ShotTraceEnd);
	}
}

void AHitScanWeapon::Multi_WeaponHitTargetBroadcast_Implementation(bool IsEnemy, bool IsHeadshot)
{
	OnWeaponHitTarget.Broadcast(IsEnemy, IsHeadshot);
}
