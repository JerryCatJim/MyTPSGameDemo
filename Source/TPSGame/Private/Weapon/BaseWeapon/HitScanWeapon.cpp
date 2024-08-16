// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon/HitScanWeapon.h"
#include "SCharacter.h"
#include "../TPSGame.h"
#include "Kismet/GameplayStatics.h"

AHitScanWeapon::AHitScanWeapon()
{
	WeaponBulletType = EWeaponBulletType::HitScan;
}

void AHitScanWeapon::DealFire()
{
	//SCharacter.cpp中重写了Pawn.cpp的GetPawnViewLocation().以获取CameraComponent的位置而不是人物Pawn的位置
	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation,EyeRotation);
	
	//伤害效果射击方位
	FVector ShotDirection = EyeRotation.Vector();
		
	//Radian 弧度
	//连续射击同一点位(不扩散时),服务器会省略一部分通信复制内容,因此让子弹扩散,保持射击轨迹同步复制
	float HalfRadian = FMath::DegreesToRadians(GetDynamicBulletSpread());
	//轴线就是传入的ShotDirection向量
	FVector NewShotDirection = FMath::VRandCone(ShotDirection, HalfRadian, HalfRadian);
		
	//射程终止点
	FVector EndPoint = EyeLocation + (NewShotDirection*WeaponTraceRange);
	//FVector TraceEnd = EyeLocation + MyOwner->GetActorForwardVector() * WeaponTraceRange
	
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
	FVector StartPoint = GetWeaponShootStartPoint(EyeLocation, EyeRotation);
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
		UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);
			
		//若击中目标则将轨迹结束点设置为击中点
		ShotTraceEnd = Hit.ImpactPoint;
		HitSurfaceType = SurfaceType;
		
		PlayImpactEffectsAndSounds(HitSurfaceType, ShotTraceEnd);
	}
}
