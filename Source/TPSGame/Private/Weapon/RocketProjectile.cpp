// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/RocketProjectile.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Weapon/SWeapon.h"

ARocketProjectile::ARocketProjectile()
{
	Damage = 120.f;
	
	//禁止Super::OnHit()中的Destroy(),手动管理销毁时机
	bDestroyOnHit = false;
	bIsAoeDamage = true;
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	//在RocketProjectile子类中手动指定TrailSystem
	if(TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false  //手动销毁，创造出火箭弹命中后有一阵浓烟未散去的效果
			);
	}
	if(ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1,
			1,
			0,
			LoopingSoundAttenuation,
			(USoundConcurrency*) nullptr,
			false
			);
	}
}

void ARocketProjectile::ApplyProjectileDamage(AActor* DamagedActor)
{
	//Super::ApplyProjectileDamage(DamagedActor, BaseDamage, EventInstigator, DamageCauser, DamageTypeClass);
	
	AController* FiringController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
	TArray<AActor*> IgnoreList;
	IgnoreList.Emplace(this);  //不无视火箭弹自己会无法造成伤害(?),看源码没看懂
	
	UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				MinDamage,
				GetActorLocation(),  //火箭弹命中时的圆心，以此向外衰减伤害
				InnerRadius,
				OuterRadius,
				1.f,
				DamageTypeClass,
				IgnoreList,  //不无视发射者自己，意味着可以炸到自己
				OwnerWeapon, //此函数会将DamageCauser自动无视
				FiringController
				);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);

	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &ARocketProjectile::DestroyTimerFinished, DestroyTime, false);
}

void ARocketProjectile::PostOnHit()
{
	Super::PostOnHit();

	if(TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if(ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void ARocketProjectile::DestroyTimerFinished()
{
	Destroy();
}
