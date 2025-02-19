// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/Projectile.h"
#include "GrenadeProjectile.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API AGrenadeProjectile : public AProjectile
{
	GENERATED_BODY()
public:

protected:
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* GrenadeMovementComponent;

private:
	UPROPERTY(EditAnywhere, Category = Projectile)
	USoundCue* BounceSound;
	
public:
	AGrenadeProjectile();

	virtual void Destroyed() override;
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	virtual void ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult) override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void PostOnHit() override;
	
	virtual void StartDestroyTimerFinished() override;
};
