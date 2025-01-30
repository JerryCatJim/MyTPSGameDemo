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
	UPROPERTY(EditAnywhere)  //伤害可以衰减到的最小伤害
	float MinDamage = 10.f;

	UPROPERTY(EditAnywhere)  //应用高伤害的内部半径
	float InnerRadius = 150.f;

	UPROPERTY(EditAnywhere)  //应用衰减伤害的外部半径
	float OuterRadius = 300.f;

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
