// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "RocketProjectile.generated.h"

/**
 * 
 */
UCLASS()
class TPSGAME_API ARocketProjectile : public AProjectile
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)  //伤害可以衰减到的最小伤害
	float MinDamage = 10.f;

	UPROPERTY(EditAnywhere)  //应用高伤害的内部半径
	float InnerRadius = 200.f;

	UPROPERTY(EditAnywhere)  //应用衰减伤害的外部半径
	float OuterRadius = 500.f;

protected:
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;
	
private:
	FTimerHandle DestroyTimerHandle;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;
	
public:
	ARocketProjectile();
	
protected:
	virtual void BeginPlay() override;

	virtual void ApplyProjectileDamage(AActor* DamagedActor) override;
	
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	virtual void PostOnHit() override;
	
	void DestroyTimerFinished();
};
