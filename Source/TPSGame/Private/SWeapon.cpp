// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "SCharacter.h"
#include "DrawDebugHelpers.h"  //射线检测显示颜色
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "../TPSGame.h"    //宏定义重命名
#include "TimerManager.h"  //定时器
#include "Net/UnrealNetwork.h"

int32 DebugWeaponDrawing = 0;
//创建自动控制台变量
FAutoConsoleVariableRef CVarDebugWeaponDrawing(TEXT("Coop.DebugWeapons"), DebugWeaponDrawing, TEXT("Draw Debug Lines For Weapon."), ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;  //SetupAttachment

	//枪口插槽名称
	MuzzleSocketName = "MuzzleSocket";

	//追踪目标名称
	TraceTargetName = "Target";

	//基础伤害
	BaseDamage = 20.f;

	//爆头伤害倍率
	HeadShotBonus = 5.f;

	//子弹扩散程度(数值会被转化为弧度,作为圆锥轴与其侧边的夹角弧度)
	BulletSpread = 0.5f;
	
	//连续射击速度(每分钟射速,不是两发之间的秒数间隔)
	RateOfShoot = 600.f;

	OnePackageAmmoNum = 30;
	CurrentAmmoNum = OnePackageAmmoNum;
	BackUpAmmoNum = 300;
	bIsReloading = false;

	WeaponName = TEXT("默认步枪");
	
	//开启网络复制，将该实体从服务器端Server复制到客户端Client
	SetReplicates(true);

	//网络更新频率
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60/RateOfShoot;
	//Owner在构造函数里拿不到，在BeginPlay里就拿到了(?)
	MyOwner = Cast<ASCharacter>(GetOwner());
}

/*// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}*/

bool ASWeapon::CheckOwnerValidAndAlive()
{
	return MyOwner && !MyOwner->GetIsDied();
}

bool ASWeapon::CheckCanFire()
{
	return CurrentAmmoNum > 0 || bIsCurrentAmmoInfinity;
}

void ASWeapon::Fire()
{
	if(!HasAuthority())  //没有主控权说明是客户端，则向服务器发送请求,在服务端调用开火函数，然后把结果复制到本地
	{
		ServerFire();
	}
	
	if(!CheckOwnerValidAndAlive())
	{
		MyOwner->SetIsFiring(false);
		return;
	}
	
	if(CurrentAmmoNum == 0 && !bIsCurrentAmmoInfinity)
	{
		//没子弹了还一直按着射击，就一直广播，用于更新子弹数UI，例如把当前子弹数颜色变成红色等
		OnCurrentAmmoChanged.Broadcast(0, true);
		MyOwner->SetIsFiring(false);
		return;
	}
	
	if(CurrentAmmoNum > 0 || bIsCurrentAmmoInfinity)
	{
		//正在装弹时触发了无限子弹BUFF时在换弹，再射击时打断换弹
		if(bIsReloading && (CurrentAmmoNum > 0 || bIsCurrentAmmoInfinity))
		{
			StopReload(true);
		}

		MyOwner->SetIsFiring(true);
		
		//SCharacter.cpp中重写了Pawn.cpp的GetPawnViewLocation().以获取CameraComponent的位置而不是人物Pawn的位置
		FVector EyeLocation;
		FRotator EyeRotation;
		
		//Character继承于Pawn, Pawn.cpp中 调用GetActorEyesViewPoint 会调用GetPawnViewLocation和GetViewRotation获得值
		MyOwner->GetActorEyesViewPoint(EyeLocation,EyeRotation);
		
		//伤害效果射击方位
		FVector ShotDirection = EyeRotation.Vector();

		float TempBulletSpread = BulletSpread;
		if(!MyOwner->GetIsAiming())  //腰射扩散大
		{
			TempBulletSpread = BulletSpread *2;
		}
		//Radian 弧度
		//连续射击同一点位(不扩散时),服务器会省略一部分通信复制内容,因此让子弹扩散,保持射击轨迹同步复制
		float HalfRadian = FMath::DegreesToRadians(TempBulletSpread);
		//轴线就是ShotDirection向量
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRadian, HalfRadian);
		
		//物理表面类型
		EPhysicalSurface MySurfaceType = SurfaceType_Default;
		
		//射程终止点
		FVector TraceEnd = EyeLocation + (ShotDirection*10000);
		//FVector TraceEnd = EyeLocation + MyOwner->GetActorForwardVector() * 10000
		
		//碰撞查询
		FCollisionQueryParams QueryParams;
		//忽略武器自身和持有者的碰撞
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
		QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质

		//射击轨迹结束点
		FVector TraceEndPoint = TraceEnd;
		
		//击中结果
		FHitResult Hit;

		//广播子弹数变化，用以更新UI等，C++服务器端不会自动调用OnRep，所以手动调用
		CurrentAmmoNum = bIsCurrentAmmoInfinity ? CurrentAmmoNum : CurrentAmmoNum - 1;
		OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, true);
	
		//射线检测
		bool bIsTraceHit;  //是否射线检测命中
		bIsTraceHit = GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, Collision_Weapon, QueryParams);
		if(bIsTraceHit)
		{
			//伤害处理

			//击中物体
			AActor* HitActor = Hit.GetActor();

			//实际伤害
			float ActualDamage = BaseDamage;
			
			//获得表面类型 PhysicalMaterial
			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());  //弱引用
			//EPhysicalSurface SurfaceType = UGameplayStatics::GetSurfaceType(Hit);

			//爆头伤害加成
			if(SurfaceType == Surface_FleshVulnerable)
			{
				ActualDamage *= HeadShotBonus;
			}

			//应用伤害
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);
			
			//若击中目标则将轨迹结束点设置为击中点
			TraceEndPoint = Hit.ImpactPoint;

			MySurfaceType = SurfaceType;

			//播放击中特效
			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);
		}

		//武器轨迹和枪口特效
		PlayFireEffects(TraceEndPoint);

		//把局部变量存到全局变量中，进行网络复制同步
		if(HasAuthority())  //HasAuthority为服务器
		{
			//轨迹终点
			HitScanTrace.TraceTo = TraceEndPoint;
			HitScanTrace.SurfaceType = MySurfaceType;
		}
		
		//记录世界时间
		LastFireTime = GetWorld()->TimeSeconds;
		
		//控制台控制是否显示
		if(DebugWeaponDrawing > 0)
		{
			//绘制射线
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 1.0f, 0,1.0f);
		}

		//播放射击动画之类的
		ServerFire_BP();
	}
}

void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
	if(MuzzleEffect)
	{
		//武器开火特效
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComponent, MuzzleSocketName);
	}

	//获取输入实际位置
	const FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);
		
	if(TraceEffect)
	{
		//弹道特效
		UParticleSystemComponent* TraceComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TraceEffect, MuzzleLocation);
		if(TraceComponent)
		{
			TraceComponent->SetVectorParameter(TraceTargetName, TraceEnd);
		}
	}

	//获得武器持有者owner
	//APawn* MyOwner = Cast<APawn>(GetOwner());
	if(MyOwner)
	{
		//获得玩家控制器
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if(PC)
		{
			//调用PlayerController.h cpp中的摄像机抖动函数
			PC->ClientStartCameraShake(FireCameraShake);
		}
	}
}

//播放武器命中效果
void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	//选中的击中效果
	UParticleSystem* SelectedEffect = nullptr;
			
	switch(SurfaceType)
	{
		//project setting中的FleshDefault和FleshVulnerable
		case Surface_FleshDefault:
		case Surface_FleshVulnerable:
			SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}
			
	if(SelectedEffect)// && ImpactPoint.Size() > 0)
	{
		const FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);

		//射击轨迹的向量
		FVector ShootDirection = ImpactPoint - MuzzleLocation;
		ShootDirection.Normalize();  //等同于Hit.ImpactNormal
		
		//击中特效
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint,  ShootDirection.Rotation());
	}
}


void ASWeapon::StartFire_Implementation()
{
	//第一次延迟时间
	float FirstDelay = LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds;
	FirstDelay = FMath::Clamp(FirstDelay, 0.f, FirstDelay);
	//FirstDelay = FMath::Max(FirstDelay, 0.f);

	//换弹时一直按着开火，换好后isFiring状态没有更新，所以最终挪到Fire()的可开火片段中设置
	//MyOwner->SetIsFiring(CheckCanFire());
	
	GetWorldTimerManager().SetTimer(TimerHandle_TimerBetweenShot, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire_Implementation()
{
	if(!HasAuthority())
	{
		ServerStopFire();
	}

	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	MyOwner->SetIsFiring(false);
	
	GetWorldTimerManager().ClearTimer(TimerHandle_TimerBetweenShot);
	if(CurrentAmmoNum == 0)
	{
		Reload(true);
	}

	ServerStopFire_BP();
}

void ASWeapon::Reload_Implementation(bool isAutoReload)
{
	if(!HasAuthority())
	{
		ServerReload(isAutoReload);
	}
	
	if(!CheckOwnerValidAndAlive() || bIsReloading || MyOwner->GetIsFiring() || CurrentAmmoNum == OnePackageAmmoNum + 1 || bIsCurrentAmmoInfinity)
	{
		return;
	}

	if(BackUpAmmoNum == 0)
	{
		//0备弹也广播，可能用于UI的备弹量数字颜色改变等小需求，C++服务器端不会自动调用OnRep，所以手动调用
		//如果当前子弹和备用子弹都空了，按开火后也会走到这里，不希望此时触发备用子弹的文字动画
		OnBackUpAmmoChanged.Broadcast(0, !isAutoReload);
		return;
	}
	
	//播放装弹动画
	bIsReloading = true;
	float MontagePlayTime = ReloadMontage->SequenceLength;//MyOwner->PlayAnimMontage(ReloadMontage);

	//Multi播放换弹动画，保证同步
	Multi_PlayReloadMontage(ReloadMontage);
	
	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("蒙太奇长度: %f"),MontagePlayTime));
	GetWorldTimerManager().SetTimer(ReloadTimer, [this](){StopReload(false);}, MontagePlayTime, false);
}

void ASWeapon::StopReload(bool IsInterrupted)
{
	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	bIsReloading = false;
	MyOwner->StopAnimMontage(ReloadMontage);

	if(!IsInterrupted)
	{
		if(CurrentAmmoNum != OnePackageAmmoNum)
		{
			const float ReloadedNum = OnePackageAmmoNum-CurrentAmmoNum > BackUpAmmoNum ? BackUpAmmoNum : OnePackageAmmoNum-CurrentAmmoNum;
			BackUpAmmoNum = bIsBackUpAmmoInfinity ? BackUpAmmoNum : BackUpAmmoNum - ReloadedNum;
			CurrentAmmoNum = CurrentAmmoNum + ReloadedNum;
		}
		else  //弹匣满了就只上一发子弹
		{
			BackUpAmmoNum = bIsBackUpAmmoInfinity ? BackUpAmmoNum : BackUpAmmoNum - 1;
			++CurrentAmmoNum;
		}
		//广播Reload事件，可用于更新子弹数UI等，C++服务器端不会自动调用OnRep，所以手动调用
		OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, false);
		OnBackUpAmmoChanged.Broadcast(BackUpAmmoNum, false);
	}
}

void ASWeapon::Multi_PlayReloadMontage_Implementation(UAnimMontage* MontageToPlay)
{
	if(CheckOwnerValidAndAlive())
	{
		MyOwner->PlayAnimMontage(MontageToPlay);
	}
}


//每当HitScanTrace这个变量被网络复制同步(在Fire函数中执行),就触发一次该函数
void ASWeapon::OnRep_HitScanTrace()
{
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ASWeapon::OnRep_CurrentAmmoNum()
{
	OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, true);
}

void ASWeapon::OnRep_BackUpAmmoNum()
{
	OnBackUpAmmoChanged.Broadcast(BackUpAmmoNum,true);
}


void ASWeapon::OnRep_IsCurrentAmmoInfinity()
{
	//从无限子弹变回有限子弹时不在UI上播放数字变形等特效
	OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, false);
}

void ASWeapon::OnRep_IsBackUpAmmoInfinity()
{
	OnBackUpAmmoChanged.Broadcast(BackUpAmmoNum, false);
}

//服务器开火函数(客户端发送开火请求，服务器调用真正的开火逻辑)
void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()  //查看代码是否存在(或用于反作弊)
{
	return true;
}

void ASWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

void ASWeapon::ServerReload_Implementation(bool isAutoReload)
{
	Reload(isAutoReload);
}

void ASWeapon::ServerFire_BP_Implementation()
{
	//ToDo In blueprint
}

void ASWeapon::ServerStopFire_BP_Implementation()
{
	//ToDo In blueprint
}

void ASWeapon::ServerReload_BP_Implementation()
{
	//ToDo In blueprint
}

//一个模板
void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);  //不让发出请求的客户端 进行自身的网络同步复制，避免重复(因为自身Client开火后, 在本地就已经绘制了自身的特效)
	DOREPLIFETIME_CONDITION(ASWeapon, CurrentAmmoNum, COND_None);
	DOREPLIFETIME_CONDITION(ASWeapon, BackUpAmmoNum, COND_None);
	DOREPLIFETIME_CONDITION(ASWeapon, bIsReloading, COND_None);
}
