// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/AdvancedWeapon/Shotgun.h"
#include "SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameMode/TPSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "TPSGameType/CustomCollisionType.h"
#include "TPSGameType/CustomSurfaceType.h"

AShotgun::AShotgun()
{
	BulletSpread = 3;
	ReloadPlayRate = 3;
	BaseDamage = 15.f;
	HeadShotBonus = 4.f;
	RateOfShoot = 100.f;
}

float AShotgun::GetDynamicBulletSpread()
{
	if(CheckOwnerValidAndAlive())
	{
		float TempRate = 2;
		if(MyOwner->GetVelocity().Size2D() > 0)
		{
			TempRate *= 2;
		}
		if(MyOwner->GetIsAiming())
		{
			TempRate *= 0.1;
		}
		if(MyOwner->GetCharacterMovement() && MyOwner->GetCharacterMovement()->IsFalling())
		{
			TempRate *= 10;
		}
		TempRate = FMath::Clamp(TempRate, 0.5f, 3.f);
		return BulletSpread * TempRate;
	}
	return BulletSpread;
}

void AShotgun::DealFire()
{
	//SCharacter.cpp中重写了Pawn.cpp的GetPawnViewLocation().以获取CameraComponent的位置而不是人物Pawn的位置
	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation,EyeRotation);
	
	//碰撞查询
	FCollisionQueryParams QueryParams;
	//忽略武器自身和持有者的碰撞
	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
	QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质

	bool bWeaponHitTarget = false;
	bool IsEnemy = false;
	
	TArray<FVector> TracePointArray;
	//基本就是把单发射击武器的DealFire逻辑，复制粘贴成次数循环的
	for(int i=0; i<NumberOfPellets; ++i)
	{
		//伤害效果射击方位
		FVector ShotDirection = EyeRotation.Vector();
		//Radian 弧度
		//连续射击同一点位(不扩散时),服务器会省略一部分通信复制内容,因此让子弹扩散,保持射击轨迹同步复制
		float HalfRadian = FMath::DegreesToRadians(GetDynamicBulletSpread());
		//轴线就是传入的ShotDirection向量
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRadian, HalfRadian);
	
		//射程终止点
		FVector EndPoint = EyeLocation + (ShotDirection*WeaponTraceRange);
		//FVector TraceEnd = EyeLocation + MyOwner->GetActorForwardVector() * WeaponTraceRange
		ShotTraceEnd = EndPoint;  //霰弹枪的ShotTraceEnd没啥用
		
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
			//记录命中事件,等待循环结束后广播
			if(HitActor)
			{
				APawn* HitTarget = Cast<APawn>(HitActor);
				if(HitTarget)
				{
					ATPSPlayerState* PS = HitTarget->GetPlayerState<ATPSPlayerState>();
					ATPSPlayerState* MyPS = MyOwner->GetPlayerState<ATPSPlayerState>();
					if(PS && MyPS)
					{
						ATPSGameMode* GM = Cast<ATPSGameMode>(UGameplayStatics::GetGameMode(this));
						if(GM && GM->GetIsTeamMatchMode())
						{
							//一次发射多发弹丸,如果同时命中友军和敌军,希望记录为命中敌军
							IsEnemy = IsEnemy || MyPS->GetTeam() != PS->GetTeam();
						}
					}
					bWeaponHitTarget = true;
				}
			}
			
			//若击中目标则将轨迹结束点设置为击中点
			ShotTraceEnd = Hit.ImpactPoint;
			HitSurfaceType = SurfaceType;
			
			PlayImpactEffectsAndSounds(HitSurfaceType, ShotTraceEnd);
		}
		TracePointArray.Emplace(ShotTraceEnd);
		//PlayTraceEffect(ShotTraceEnd);  //换个写法，最好别一瞬间执行多次RPC调用，尤其还是Reliable函数
	}

	if(bWeaponHitTarget)
	{
		//将击中事件广播出去，可用于HitFeedBackCrossHair这个UserWidget播放击中特效等功能
		Multi_WeaponHitTargetBroadcast(IsEnemy);
	}
	
	PlayTraceEffectForShotgun(TracePointArray);
}

void AShotgun::PlayTraceEffectForShotgun_Implementation(const TArray<FVector>& TracePointArray)
{
	for(const FVector& TracePoint : TracePointArray)
	{
		DealPlayTraceEffect(TracePoint);
	}
}