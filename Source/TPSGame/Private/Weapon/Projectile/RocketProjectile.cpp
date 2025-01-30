// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile/RocketProjectile.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Weapon/Component/RocketMovementComponent.h"
#include "Weapon/BaseWeapon/SWeapon.h"

ARocketProjectile::ARocketProjectile()
{
	Damage = 120.f;
	CollisionBox->InitBoxExtent(FVector(15,3,3));
	
	bIsAoeDamage = true;

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
	RocketMovementComponent->ProjectileGravityScale = 0.f;

	RocketMovementComponent->InitialSpeed = 1500.f;
	RocketMovementComponent->MaxSpeed = 1500.f;

	//命中后需要延迟销毁火箭弹以造成烟雾逐渐散去的效果(效果不好，现在改成0立刻销毁了)
	OnHitDestroyTime = 0.f;
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();
	
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
	//原本有逻辑，都优化整合到基类中了
}

void ARocketProjectile::PostOnHit()
{
	Super::PostOnHit();

	RocketMovementComponent->Deactivate();
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void ARocketProjectile::StartDestroyTimerFinished()
{
	//到达生命周期，触发一次空爆
	OnHit(nullptr,nullptr,nullptr,FVector(),FHitResult());
}

void ARocketProjectile::Destroyed()
{
	Super::Destroyed();
	
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}