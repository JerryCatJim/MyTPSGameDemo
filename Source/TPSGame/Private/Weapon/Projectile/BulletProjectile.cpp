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
	//在面对后方时向前方射击，子弹有较低概率会打中自己(因为当时的枪口在身体后方)，配合BulletMovementComponent在打中自己时继续飞行而不是停在空中
	if(OtherActor == GetInstigator())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, TEXT("Bullet Hit Self."));
		return;
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void ABulletProjectile::ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult)
{
	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr ;
	//应用伤害
	UGameplayStatics::ApplyPointDamage(DamagedActor, ActualDamage, HitFromDirection, HitResult, InstigatorController, OwnerWeapon, DamageTypeClass);
}
