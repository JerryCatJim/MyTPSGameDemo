// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"

#include "SCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "TPSGame/TPSGame.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SetReplicates(true);
	SetReplicatingMovement(true);
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(Collision_Projectile);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if(Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
			);
	}

	//在构造函数中绑定可能有些问题
	if(HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	//获得表面类型 PhysicalMaterial
	//EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());  //弱引用
	EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(Hit);
	FVector HitLocation = GetActorLocation();
	
	if(Cast<ASCharacter>(OtherActor))
	{
		//PlayerCharacter的Hit碰撞检测有些问题，没法精确碰撞，所以命中后再做一下射线检测获取命中区域的材质
		//碰撞查询
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
		QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质
		//击中结果
		FHitResult NewHit;
		//射线检测
		bool bIsTraceHit;  //是否射线检测命中
		bIsTraceHit = GetWorld()->LineTraceSingleByChannel(NewHit, GetActorLocation(), GetActorLocation() + GetActorForwardVector()*100, Collision_Weapon, QueryParams);
		if(bIsTraceHit)
		{
			SurfaceType = UGameplayStatics::GetSurfaceType(NewHit);
			HitLocation = Hit.ImpactPoint;
		}
	}
	PlayImpactEffectsAndSounds(SurfaceType, HitLocation);
	Destroy();
}

void AProjectile::PlayImpactEffectsAndSounds_Implementation(EPhysicalSurface SurfaceType, FVector HitLocation)
{
	UParticleSystem* SelectedEffect;
	USoundCue* ImpactSound;
	
	switch(SurfaceType)
	{
		//project setting中的FleshDefault和FleshVulnerable
		case Surface_FleshDefault:      //身体
		case Surface_FleshVulnerable:   //易伤部位，例如爆头，可单独设置特效和音效
			SelectedEffect = FleshImpactEffect;
			ImpactSound = FleshHitSound;
			break;
		default:
			SelectedEffect = DefaultImpactEffect;
			ImpactSound = DefaultHitSound;
			break;
	}
			
	if(SelectedEffect)// && ImpactPoint.Size() > 0)
	{
		//击中特效
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, HitLocation, HitLocation.Rotation());
	}
	if(ImpactSound)
	{
		//播放命中音效
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, HitLocation);
	}
}
