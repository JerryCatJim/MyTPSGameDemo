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
	void Multi_PostOnHit();
	
	//命中后的处理函数
	virtual void PostOnHit();  //将具体内容从Multi中剥离，防止如果想继承修改Multi函数导致多次调用
	
	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation);
	
	void PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation);  //将具体内容从Multi中剥离，防止如果想继承修改Multi函数导致多次调用

	//UFUNCTION(Server, Reliable)  //OnHit中调用应用伤害时检测是否有服务器权限
	virtual void ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult);
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 20.f;
	
protected:
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;
	
	UPROPERTY()
	class ASWeapon* OwnerWeapon;

	float HeadShotBonusRate = 1;

	UPROPERTY()
	TSubclassOf<UDamageType> DamageTypeClass;
	
	//是否在命中时直接销毁自身
	UPROPERTY(EditAnywhere)
	bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere)
	bool bIsAoeDamage = false;

	//被发射时的时间，配合命中时的时间来做例如刚发射时就因碰撞问题Hit到自己还是发射了一段时间后才Hit自己
	float SpawnTime = 0;
	float OnHitTime = 0;
	float LastOnHitTime = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  //子弹发射的x秒内无法命中发射者自身
	float CannotHitOwnerTimeAfterSpawn = 0.5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)  //子弹是否可以命中发射者自身
	bool bCanHitOwner = true;

	bool bOnHitSuccessful = false;  //鉴于上面设定的子弹在一定时间内可以多次碰撞发射者，记录真正完成有效Hit的结果，以供子类做一些逻辑判断判断
	
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
