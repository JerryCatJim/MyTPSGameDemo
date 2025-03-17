// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile/Projectile.h"

#include "SCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "NiagaraFunctionLibrary.h"
#include "TPSGameState.h"
#include "Component/SkillComponent.h"
#include "TPSGameType/CustomCollisionType.h"
#include "TPSGameType/CustomSurfaceType.h"

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

	//射出子弹后立刻更换武器可能导致OwnerWeapon被销毁(与地面武器交换或丢弃等操作)，所以会取不到GetHeadShotBonus()，因此提前记录，然后在OnHit爆头时直接取用数值防止崩溃
	HeadShotBonusRate = OwnerWeapon ? OwnerWeapon->GetHeadShotBonus() : 1;
	
	StartDestroyTimer();
	SpawnTracerAndTrailSystem();
	
	//在构造函数中绑定可能有些问题
	if(HasAuthority())
	{
		//Bullet型Projectile的OnHit()在击中离人物很近的位置时，只有服务器响应了OnHit，客户端没响应(?)，击中远一点的地方时都响应(?)，所以还是改回只在服务器响应OnHit，使用Multicast同步特效等
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

		SpawnTime = GetWorld()->GetTimeSeconds();
	}
}

void AProjectile::StartDestroyTimer()
{
	if(GetWorld())
	{
		//子弹被生成时开启X秒后主动销毁子弹的计时器
		GetWorldTimerManager().SetTimer(StartDestroyTimerHandle, this, &AProjectile::StartDestroyTimerFinished, StartDestroyTime, false);
	}
}

void AProjectile::SpawnTracerAndTrailSystem()
{
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

	//在RocketProjectile等子类中手动指定TrailSystem
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
}

//后续可添加子弹忽视友军/开启友军伤害选项功能
void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//获得表面类型 PhysicalMaterial
	//EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());  //弱引用
	EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(Hit);
	FVector HitLocation = GetActorLocation();

	LastOnHitTime = OnHitTime;
	OnHitTime = GetWorld()->GetTimeSeconds();
	
	//在面对后方时向前方射击，BulletProjectile子类有较低概率会打中自己(因为当时的枪口在身体后方)，配合BulletMovementComponent在打中自己时会停在空中等待(?难道需要手动处理让子弹向前瞬移一段距离还是怎么)
	//RocketProjectile子类同理
	if(HasAuthority())
	{
		if(OtherActor == GetInstigator() && bCanHitOwner )
		{
			if(OnHitTime - SpawnTime <= 0)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
					FString::Printf(TEXT("子弹OnHit时间小于等于Spawn时间,时间之差为: %f, 可能为刚生成就碰撞到自身,建议调整MuzzleFlash插槽位置."),
						OnHitTime - SpawnTime));
				return;
			}
			if(OnHitTime - SpawnTime <= CannotHitOwnerTimeAfterSpawn )  //假设发射后很短时间(或自定义时间)内就击中了自身,则试图让子弹穿过自身
			{
				GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, TEXT("Bullet Hit Self."));
				SetActorLocation(GetActorLocation() + GetActorForwardVector()*GetWorld()->GetDeltaSeconds()*GetVelocity().Size());
				return;
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Blue, TEXT("Bullet Damage Self."));
			}
		}
	}
	
	if(HasAuthority()) //应用伤害效果只能在服务器
	{
		AActor* DamagedActor = OtherActor;
		float ActualDamage = Damage;
		FVector ShotDirection = FVector();
		FHitResult NewHit;

		bool IsHeadshot = false;
		
		if(OwnerWeapon)
		{
			FVector EyeLocation;
			FRotator EyeRotation;
			//射出子弹后立刻更换武器可能导致OwnerWeapon被销毁(与地面武器交换或丢弃等操作)，所以会取不到GetOwner导致空指针游戏崩溃
			//如果射出子弹后，命中前开枪人物立刻被销毁也可能崩溃，所以做好边界限制
			if(OwnerWeapon->GetOwner())
			{
				OwnerWeapon->GetOwner()->GetActorEyesViewPoint(EyeLocation,EyeRotation);
				//伤害效果射击方位(只有TakePointDamage会用到)
				ShotDirection = EyeRotation.Vector();
			}
		}
		
		if(!bIsAoeDamage)  //火箭筒是Aoe伤害，不用做单点射线检测
		{
			//OnComponentHit碰撞检测有些问题，拿不到命中材质，所以命中后再做一下射线检测获取命中区域的材质
			//碰撞查询
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
			QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质
			//射线检测
			bool bIsTraceHit;  //是否射线检测命中
			bIsTraceHit = GetWorld()->LineTraceSingleByChannel(NewHit, GetActorLocation(), GetActorLocation() + GetActorForwardVector()*100, Collision_Weapon, QueryParams);
			if(bIsTraceHit)
			{
				DamagedActor = Hit.GetActor();
				SurfaceType = UGameplayStatics::GetSurfaceType(NewHit);
				HitLocation = Hit.ImpactPoint;
				
				//爆头伤害加成
				if(SurfaceType == Surface_FleshVulnerable)
				{
					ActualDamage *= HeadShotBonusRate;
					IsHeadshot = true;
				}
			}
		}

		//广播命中事件
		if(DamagedActor)
		{
			bool IsEnemy = true;
			APawn* HitTarget = Cast<APawn>(DamagedActor);
			if(HitTarget && OwnerWeapon)
			{
				ASCharacter* MyOwner = Cast<ASCharacter>(OwnerWeapon->GetOwner());
				if(MyOwner)
				{
					ATPSPlayerState* PS = HitTarget->GetPlayerState<ATPSPlayerState>();
					ATPSPlayerState* MyPS = MyOwner->GetPlayerState<ATPSPlayerState>();
					if(PS && MyPS)
					{
						ATPSGameState* GS = Cast<ATPSGameState>(UGameplayStatics::GetGameState(this));
						if(GS && GS->GetIsTeamMatchMode())
						{
							IsEnemy = MyPS->GetTeam() != PS->GetTeam();
						}
					}
				}
				//将击中事件广播出去，可用于HitFeedBackCrossHair这个UserWidget播放击中特效等功能
				Multi_WeaponHitTargetBroadcast(IsEnemy, IsHeadshot && !bIsAoeDamage);
				if(MyOwner->GetSkillComponent() && OwnerWeapon)
				{
					float AddPercent = IsHeadshot ? OwnerWeapon->GetHitChargePercent() * OwnerWeapon->GetHitChargeHeadshotBonus() : OwnerWeapon->GetHitChargePercent();
					MyOwner->GetSkillComponent()->AddSkillChargePercent(AddPercent);
				}
			}
		}
		
		ApplyProjectileDamage(DamagedActor, ActualDamage, ShotDirection, NewHit);
		Multi_PlayImpactEffectsAndSounds(SurfaceType, HitLocation);
		Multi_PostOnHit();
	}
}

void AProjectile::Multi_WeaponHitTargetBroadcast_Implementation(bool IsEnemy, bool IsHeadshot)
{
	if(OwnerWeapon)
	{
		OwnerWeapon->OnWeaponHitTarget.Broadcast(IsEnemy, IsHeadshot);
	}
}

void AProjectile::Multi_PostOnHit_Implementation()
{
	PostOnHit();
}

void AProjectile::PostOnHit()
{
	//命中后延迟X秒销毁，用于例如创造出火箭弹命中后有一阵浓烟未散去的效果
	if(OnHitDestroyTime <= 0)
	{
		//Timer的 InRate <=0 时会清除Timer而不是触发
		OnHitDestroyTimerFinished();
	}
	else if(GetWorld())
	{
		GetWorldTimerManager().SetTimer(OnHitDestroyTimerHandle, this, &AProjectile::OnHitDestroyTimerFinished, OnHitDestroyTime, false);
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

void AProjectile::StartDestroyTimerFinished()
{
	if(GetWorld())
	{
		GetWorldTimerManager().ClearTimer(OnHitDestroyTimerHandle);
	}
	Destroy();
}

void AProjectile::OnHitDestroyTimerFinished()
{
	if(GetWorld())
	{
		GetWorldTimerManager().ClearTimer(StartDestroyTimerHandle);
	}
	Destroy();
}

void AProjectile::Multi_PlayImpactEffectsAndSounds_Implementation(EPhysicalSurface SurfaceType, FVector HitLocation)
{
	PlayImpactEffectsAndSounds(SurfaceType, HitLocation);
}

void AProjectile::ApplyProjectileDamage(AActor* DamagedActor, float ActualDamage, const FVector& HitFromDirection, const FHitResult& HitResult)
{
	//在子类中实现
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

void AProjectile::Destroyed()
{
	//弹头被销毁时粒子系统需要手动关闭
	if(TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
}
