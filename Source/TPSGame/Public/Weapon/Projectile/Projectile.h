// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Projectile.generated.h"

UCLASS()
class TPSGAME_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

	virtual void Destroyed() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multi_PostOnHit();
	
	//命中后的处理函数
	virtual void PostOnHit();  //将具体内容从Multi中剥离，防止如果想继承修改Multi函数导致多次调用

	virtual void StartDestroyTimerFinished();  //子弹生成x秒后完成生命周期主动销毁时触发的函数
	virtual void OnHitDestroyTimerFinished();  //子弹命中物体后延迟x秒后销毁时触发的函数
	
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
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent;
	
	UPROPERTY()
	class ASWeapon* OwnerWeapon;

	float HeadShotBonusRate = 1;

	UPROPERTY()
	TSubclassOf<UDamageType> DamageTypeClass;
	
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

	//子弹尾部轨迹特效
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;
	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	//子弹刚被生成时就启动的计时器，X秒后主动销毁子弹
	FTimerHandle StartDestroyTimerHandle;
	UPROPERTY(EditAnywhere, meta=(ClampMin = 0.5f, ClampMax = 99.9f))
	float StartDestroyTime = 5.f;  //子类自行设定生成子弹后多少秒销毁
	
	//命中后延迟销毁的计时器
	FTimerHandle OnHitDestroyTimerHandle;
	UPROPERTY(EditAnywhere)
	float OnHitDestroyTime = 0.f;  //子类自行设定命中后延迟多少秒销毁
	
private:
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
