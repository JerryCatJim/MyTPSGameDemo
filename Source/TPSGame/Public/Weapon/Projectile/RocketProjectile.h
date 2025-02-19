// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile/Projectile.h"
#include "RocketProjectile.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ARocketProjectile : public AProjectile
{
	GENERATED_BODY()
public:

protected:
	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
	
public:
	ARocketProjectile();

	virtual void Destroyed() override;
	
protected:
	virtual void BeginPlay() override;

	virtual void ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult) override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void PostOnHit() override;
	
	virtual void StartDestroyTimerFinished() override;
};
