// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"

#include "SCharacter.h"
#include "Components/BoxComponent.h"
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

	//重设碰撞盒大小，先改为(2,1,1)
	CollisionBox->InitBoxExtent(FVector(2,1,1));
	
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	OwnerWeapon = Cast<ASWeapon>(GetOwner());
	DamageTypeClass = OwnerWeapon ? OwnerWeapon->GetWeaponDamageType() : UDamageType::StaticClass();
	
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
	//if(HasAuthority())
	//{
		//改为双端绑定OnHit,方便双端同步一些效果
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	//}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//获得表面类型 PhysicalMaterial
	//EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());  //弱引用
	EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(Hit);
	FVector HitLocation = GetActorLocation();

	ASCharacter* DamagedActor = Cast<ASCharacter>(OtherActor);
	bool CanApplyDamage = (!bIsAoeDamage && DamagedActor) || bIsAoeDamage;
	if(CanApplyDamage && HasAuthority()) //应用伤害效果只能在服务器
	{
		if(!bIsAoeDamage)  //火箭筒是Aoe伤害，不用做单点射线检测
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
		ApplyProjectileDamage(DamagedActor);
	}
	Multi_PlayImpactEffectsAndSounds(SurfaceType, HitLocation);
	Multi_PostOnHit();
}

void AProjectile::Multi_PostOnHit()//_Implementation()
{
	PostOnHit();
}

void AProjectile::PostOnHit()
{
	//子类RocketProjectile期望命中后延迟销毁来创造浓雾，所以在其类中设为false
	if(bDestroyOnHit)
	{
		Destroy();
	}
	
	//命中后隐藏Mesh
	if(MeshComponent)
	{
		MeshComponent->SetVisibility(false);
	}
	if(CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AProjectile::Multi_PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation)//_Implementation(EPhysicalSurface SurfaceType, FVector HitLocation)
{
	PlayImpactEffectsAndSounds(SurfaceType, HitLocation);
}

void AProjectile::ApplyProjectileDamage(AActor* DamagedActor)
{
	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr ;
	//应用伤害
	UGameplayStatics::ApplyDamage(DamagedActor, Damage, InstigatorController, OwnerWeapon, DamageTypeClass);
}

void AProjectile::PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector HitLocation)
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
