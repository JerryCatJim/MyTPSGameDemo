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
	
	//UFUNCTION(NetMulticast, Reliable)
	void Multi_PostOnHit();
	
	//命中后的处理函数
	virtual void PostOnHit();  //将具体内容从Multi中剥离，防止如果想继承修改Multi函数导致多次调用
	
	//UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation);
	
	void PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation);  //将具体内容从Multi中剥离，防止如果想继承修改Multi函数导致多次调用

	//UFUNCTION(Server, Reliable)  OnHit中调用应用伤害时检测是否有服务器权限
	virtual void ApplyProjectileDamage(AActor* DamagedActor);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;
	
protected:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;
	
	UPROPERTY()
	class ASWeapon* OwnerWeapon;

	UPROPERTY()
	TSubclassOf<UDamageType> DamageTypeClass;
	
	//是否在命中时直接销毁自身
	UPROPERTY(EditAnywhere)
	bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere)
	bool bIsAoeDamage = false;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent;
	
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
