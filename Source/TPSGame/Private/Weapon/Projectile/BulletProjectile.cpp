// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile/BulletProjectile.h"

#include "Kismet/GameplayStatics.h"
#include "Weapon/BaseWeapon/SWeapon.h"
#include "Weapon/Component/BulletMovementComponent.h"

ABulletProjectile::ABulletProjectile()
{
	BulletMovementComponent = CreateDefaultSubobject<UBulletMovementComponent>(TEXT("BulletMovementComponent"));
	BulletMovementComponent->bRotationFollowsVelocity = true;
	BulletMovementComponent->SetIsReplicated(true);

	BulletMovementComponent->InitialSpeed = 15000.f;
	BulletMovementComponent->MaxSpeed = 15000.f;
}

void ABulletProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
	
	//if(bOnHitSuccessful)  //发射物有可能碰撞到发射者自身并不产生碰撞行为，此时视为没发生过碰撞，所以需要判断真正碰撞后才开始一些自定义逻辑
	//{
	//	//
	//}
}

void ABulletProjectile::ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult)
{
	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr ;
	//应用伤害
	UGameplayStatics::ApplyPointDamage(DamagedActor, ActualDamage, HitFromDirection, HitResult, InstigatorController, OwnerWeapon, DamageTypeClass);
}
