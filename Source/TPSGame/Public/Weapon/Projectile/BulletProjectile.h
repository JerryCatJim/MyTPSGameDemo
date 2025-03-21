// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/Projectile.h"
#include "BulletProjectile.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ABulletProjectile : public AProjectile
{
	GENERATED_BODY()

public:
	ABulletProjectile();

protected:
	virtual void BeginPlay() override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult) override;
public:
	
protected:
	UPROPERTY(VisibleAnywhere)
	class UBulletMovementComponent* BulletMovementComponent;
};
