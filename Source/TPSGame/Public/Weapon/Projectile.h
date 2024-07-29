// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class TPSGAME_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;
	
protected:

private:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	UPROPERTY(EditAnywhere)
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditAnywhere)
	class USoundCue* DefaultHitSound;

	UPROPERTY(EditAnywhere)
	class USoundCue* FleshHitSound;
};
