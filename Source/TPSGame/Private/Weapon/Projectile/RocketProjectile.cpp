// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile/RocketProjectile.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Weapon/Component/RocketMovementComponent.h"
#include "Weapon/BaseWeapon/SWeapon.h"

ARocketProjectile::ARocketProjectile()
{
	Damage = 120.f;
	CollisionBox->InitBoxExtent(FVector(15,3,3));
	
	//禁止Super::OnHit()中的Destroy(),手动管理销毁时机
	bDestroyOnHit = false;
	bIsAoeDamage = true;

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->ProjectileGravityScale = 0.f;

	RocketMovementComponent->InitialSpeed = 1500.f;
	RocketMovementComponent->MaxSpeed = 1500.f;
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	//在RocketProjectile子类中手动指定TrailSystem
	if(TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false  //手动销毁，创造出火箭弹命中后有一阵浓烟未散去的效果
			);
	}
	if(ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1,
			1,
			0,
			LoopingSoundAttenuation,
			(USoundConcurrency*) nullptr,
			false
			);
	}
}

void ARocketProjectile::ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult)
{
	//Super::ApplyProjectileDamage(DamagedActor, BaseDamage, EventInstigator, DamageCauser, DamageTypeClass);
	
	AController* FiringController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
	
	TArray<AActor*> IgnoreList;
	IgnoreList.Emplace(this);  //不无视火箭弹自己会无法造成伤害(?),看源码没看懂
	
	UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				MinDamage,
				GetActorLocation(),  //火箭弹命中时的圆心，以此向外衰减伤害
				InnerRadius,
				OuterRadius,
				1.f,
				DamageTypeClass,
				IgnoreList,  //不无视发射者自己，意味着可以炸到自己
				OwnerWeapon, //此函数会将DamageCauser自动无视
				FiringController
				);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);

	if(bOnHitSuccessful)  //发射物有可能碰撞到发射者自身并不产生碰撞行为，此时视为没发生过碰撞，所以需要判断真正碰撞后才开始计时器
	{
		GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ARocketProjectile::DestroyTimerFinished, DestroyTime, false);
	}
}

void ARocketProjectile::PostOnHit()
{
	Super::PostOnHit();

	if(TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void ARocketProjectile::DestroyTimerFinished()
{
	Destroy();
}

void ARocketProjectile::Destroyed()
{
	Super::Destroyed();

	//防止弹头突然被销毁时没有同步关闭粒子系统
	if(TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}