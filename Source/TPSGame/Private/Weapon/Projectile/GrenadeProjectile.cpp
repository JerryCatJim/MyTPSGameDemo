// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile/GrenadeProjectile.h"

#include "SCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Weapon/Component/BulletMovementComponent.h"
#include "Weapon/BaseWeapon/SWeapon.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	Damage = 100.f;
	CollisionBox->InitBoxExtent(FVector(5,2,2));
	
	bIsAoeDamage = true;

	GrenadeMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("GrenadeMovementComponent"));
	GrenadeMovementComponent->bRotationFollowsVelocity = true;
	GrenadeMovementComponent->SetIsReplicated(true);
	
	GrenadeMovementComponent->InitialSpeed = WasOverrideFromWeapon ? InitialSpeed : 1500.f;
	GrenadeMovementComponent->MaxSpeed = WasOverrideFromWeapon ? MaxSpeed : 1500.f;
	GrenadeMovementComponent->ProjectileGravityScale = WasOverrideFromWeapon ? GravityZScale : 0.5f;
	
	GrenadeMovementComponent->bShouldBounce = true;

	//命中后需要延迟销毁榴弹以造成烟雾逐渐散去的效果(效果不好，现在改成0立刻销毁了)
	OnHitDestroyTime = 0.f;

	StartDestroyTime = 1.8f;
}

void AGrenadeProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	GrenadeMovementComponent->InitialSpeed = WasOverrideFromWeapon ? InitialSpeed : 1500.f;
	GrenadeMovementComponent->MaxSpeed = WasOverrideFromWeapon ? MaxSpeed : 1500.f;
	GrenadeMovementComponent->ProjectileGravityScale = WasOverrideFromWeapon ? GravityZScale : 0.5f;
	
	GrenadeMovementComponent->OnProjectileBounce.AddDynamic(this, &AGrenadeProjectile::OnBounce);
}

void AGrenadeProjectile::ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult)
{
	//Super::ApplyProjectileDamage(DamagedActor, BaseDamage, EventInstigator, DamageCauser, DamageTypeClass);
	
	AController* FiringController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
	
	TArray<AActor*> IgnoreList;
	IgnoreList.Emplace(this);  //不无视榴弹自己会无法造成伤害(?),看源码没看懂
	
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

void AGrenadeProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//直接命中人物时才引爆，否则榴弹会反弹
	if(!Cast<ASCharacter>(OtherActor)) return;
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AGrenadeProjectile::PostOnHit()
{
	Super::PostOnHit();

	GrenadeMovementComponent->Deactivate();
}

void AGrenadeProjectile::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if(BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
	}
}

void AGrenadeProjectile::StartDestroyTimerFinished()
{
	//到达生命周期，触发一次空爆
	Super::OnHit(nullptr,nullptr,nullptr,FVector(),FHitResult());
}

void AGrenadeProjectile::Destroyed()
{
	Super::Destroyed();
	
}