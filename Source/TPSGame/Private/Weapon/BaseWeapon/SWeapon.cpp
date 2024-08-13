// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon/SWeapon.h"
#include "SCharacter.h"
#include "DrawDebugHelpers.h"  //射线检测显示颜色
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "../TPSGame.h"    //宏定义重命名
#include "TimerManager.h"  //定时器
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

int32 DebugWeaponDrawing = 0;
//创建自动控制台变量
FAutoConsoleVariableRef CVarDebugWeaponDrawing(TEXT("Coop.DebugWeapons"), DebugWeaponDrawing, TEXT("Draw Debug Lines For Weapon."), ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponBulletType = EWeaponBulletType::HitScan;
	
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	WeaponSoundAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("WeaponSoundAudio"));
	WeaponSoundAudio->SetupAttachment(RootComponent);
	WeaponSoundAudio->bAutoActivate = false;

	//枪口插槽名称
	MuzzleSocketName = "MuzzleFlash";

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
	OnceReloadAmmoNum = OnePackageAmmoNum;

	WeaponName = TEXT("默认步枪");
	
	ZoomedFOV = 45.f;
	ZoomInterpSpeed = 20.0f;
	
	//开启网络复制，将该实体从服务器端Server复制到客户端Client
	SetReplicates(true);
	SetReplicatingMovement(true);

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

	WeaponPickUpInfo.Owner = MyOwner;
	WeaponPickUpInfo.WeaponClass = GetClass();
	WeaponPickUpInfo.WeaponMesh = GetWeaponMeshComp()->SkeletalMesh;
	WeaponPickUpInfo.CurrentAmmo = CurrentAmmoNum;
	WeaponPickUpInfo.BackUpAmmo = BackUpAmmoNum;
	WeaponPickUpInfo.WeaponName = WeaponName;
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

bool ASWeapon::IsProjectileWeapon()
{
	return WeaponBulletType == EWeaponBulletType::Projectile;
}

bool ASWeapon::CheckCanFire()
{
	return CurrentAmmoNum > 0 || bIsCurrentAmmoInfinity;
}

//不想每次子弹变化都更新Info,因为要网络同步,所以在Get时才更新
FWeaponPickUpInfo ASWeapon::GetWeaponPickUpInfo()
{
	WeaponPickUpInfo.CurrentAmmo = CurrentAmmoNum;
	WeaponPickUpInfo.BackUpAmmo = BackUpAmmoNum;
	return WeaponPickUpInfo;
}

void ASWeapon::RefreshWeaponInfo_Implementation(FWeaponPickUpInfo WeaponInfo)
{
	WeaponPickUpInfo = WeaponInfo;
	
	WeaponName = WeaponInfo.WeaponName;
	GetWeaponMeshComp()->SetSkeletalMesh(WeaponInfo.WeaponMesh);
	CurrentAmmoNum = WeaponInfo.CurrentAmmo;
	BackUpAmmoNum = WeaponInfo.BackUpAmmo;
}

float ASWeapon::GetDynamicBulletSpread()
{
	if(CheckOwnerValidAndAlive())
	{
		float TempRate = 3;
		if(MyOwner->GetVelocity().Size2D() > 0)
		{
			TempRate *= 4;
		}
		if(MyOwner->GetIsAiming())
		{
			TempRate *= 0.1;
		}
		if(MyOwner->GetCharacterMovement() && MyOwner->GetCharacterMovement()->IsFalling())
		{
			TempRate *= 10;
		}
		TempRate = FMath::Clamp(TempRate, 0.5f, 15.0f);
		return BulletSpread * TempRate;
	}
	return BulletSpread;
}

bool ASWeapon::CheckIsFullAmmo()
{
	return (CurrentAmmoNum >= OnePackageAmmoNum && !CanOverloadAmmo)
		|| (CurrentAmmoNum >= OnePackageAmmoNum+1 && CanOverloadAmmo);
}

FVector ASWeapon::GetWeaponShootStartPoint(FVector EyeLocation, FRotator EyeRotation)
{
	//将开枪的射线检测起始点从摄像机前移，防止敌人在自己后面但在摄像机前面时也被射中
	float DistanceFromCamera = IsValid(MyOwner) ? MyOwner->GetSpringArmLength() + MyOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0 ;
	return EyeLocation + FTransform(FQuat(EyeRotation),EyeLocation).GetUnitAxis(EAxis::X) * DistanceFromCamera;
}

FVector ASWeapon::GetCurrentAimingPoint(bool bUseSpread)
{
	//SCharacter.cpp中重写了Pawn.cpp的GetPawnViewLocation().以获取CameraComponent的位置而不是人物Pawn的位置
	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation,EyeRotation);
	
	//伤害效果射击方位
	FVector ShotDirection = EyeRotation.Vector();
	//Radian 弧度
	//矫正武器枪口指向位置时不想应用散布导致偏移
	float HalfRadian = bUseSpread ? FMath::DegreesToRadians(GetDynamicBulletSpread()) : 0;
	//轴线就是传入的ShotDirection向量
	ShotDirection = FMath::VRandCone(ShotDirection, HalfRadian, HalfRadian);
	
	//射线检测的最远位置
	const FVector EndPoint = EyeLocation + (ShotDirection * WeaponTraceRange);
	//碰撞查询
	FCollisionQueryParams QueryParams;
	//忽略武器自身和持有者的碰撞
	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
	QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质
	//击中结果
	FHitResult Hit;
	//射线检测
	bool bIsTraceHit;  //是否射线检测命中
	FVector StartPoint = GetWeaponShootStartPoint(EyeLocation, EyeRotation);
	bIsTraceHit = GetWorld()->LineTraceSingleByChannel(Hit, StartPoint, EndPoint, Collision_Weapon, QueryParams);
	if(bIsTraceHit)
	{
		return Hit.ImpactPoint;
	}
	return EndPoint;
}

void ASWeapon::DealFire()
{
	//射线检测逻辑挪到了子类HitScanWeapon中
}

void ASWeapon::Fire()
{
	if(!HasAuthority())  //没有主控权说明是客户端，则向服务器发送请求,在服务端调用开火函数，然后把结果复制到本地
	{
		ServerFire();
		LocalFire();
		return;
	}
	
	if(!CheckOwnerValidAndAlive())
	{
		StopFire();  //不调用StopFire()会一直尝试射击，所以停掉Timer
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
		if(MyOwner->GetIsReloading() && (CurrentAmmoNum > 0 || bIsCurrentAmmoInfinity))
		{
			StopReload(true);
		}

		MyOwner->SetIsFiring(true);

		CurrentAmmoNum = bIsCurrentAmmoInfinity ? CurrentAmmoNum : CurrentAmmoNum - 1;
		ClientSyncCurrentAmmoOnFiring(CurrentAmmoNum);
		//广播Reload事件，可用于更新子弹数UI等，C++服务器端手动调用一下子弹数更新委托
		UpdateCurrentAmmoChange(true);
		
		//处理射击判定和应用伤害的函数
		DealFire();
		
		//播放特效
		PlayFireEffectsAndSounds();
		if(WeaponType != EWeaponType::ShotGun)
		{
			//霰弹枪一次可以射出多发弹丸，在其重写的DealFire()中描绘轨迹，独头霰弹枪也是
			PlayTraceEffect(ShotTraceEnd);
		}
		
		//记录世界时间
		LastFireTime = GetWorld()->TimeSeconds;
		
		//控制台控制是否显示
		if(DebugWeaponDrawing > 0)
		{
			//SCharacter.cpp中重写了Pawn.cpp的GetPawnViewLocation().以获取CameraComponent的位置而不是人物Pawn的位置
			FVector EyeLocation;
			FRotator EyeRotation;
			MyOwner->GetActorEyesViewPoint(EyeLocation,EyeRotation);
			//绘制射线
			DrawDebugLine(GetWorld(), EyeLocation, ShotTraceEnd, FColor::Red, false, 1.0f, 0,1.0f);
		}

		//播放射击动画
		PlayFireAnim();
		
		//要处理啥，留个接口出来
		PostFire();
	}
}

void ASWeapon::LocalFire()
{
	//把不影响同步的一些效果函数从Multi改成普通函数，并在本地客户端也执行，增强高延迟下的客户端体验
	PlayFireEffectsAndSounds();

	//子弹预测机制，取消CurrentAmmo的Replicated，改为手动记录因延迟而产生的未同步的差值
	if(CurrentAmmoNum > 0)
	{
		--CurrentAmmoNum;
		OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, true);
		++AmmoSequence;
	}
}

void ASWeapon::PlayFireEffectsAndSounds()//_Implementation()
{
	if(!HasAuthority())
	{
		//本地预测,防止延迟过高时开枪声音触发次数大于了当前子弹数
		if(CurrentAmmoNum <=0 ) return;
	}
	
	if(MuzzleEffect && bShowMuzzleFlash)
	{
		//武器开火特效
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComponent, MuzzleSocketName);
	}

	if(FireSound)
	{
		WeaponSoundAudio->SetSound(FireSound);
		WeaponSoundAudio->Play();
	}

	//获得武器持有者owner
	//APawn* MyOwner = Cast<APawn>(GetOwner());
	if(MyOwner && MyOwner->IsLocallyControlled())
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

void ASWeapon::PlayTraceEffect_Implementation(FVector TraceEnd)
{
	DealPlayTraceEffect(TraceEnd);
}

//播放武器命中效果
void ASWeapon::PlayImpactEffectsAndSounds_Implementation(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	DealPlayImpactEffectsAndSounds(SurfaceType, ImpactPoint);
}

void ASWeapon::DealPlayTraceEffect(FVector TraceEnd)
{
	if(TraceEffect && !IsProjectileWeapon())  //发射器类武器不播放轨迹特效，因为轨迹特效是瞬间描绘的
	{
		//获取输入实际位置
		const FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);
		//弹道特效
		UParticleSystemComponent* TraceComponent = UGameplayStatics::SpawnEmitterAtLocation(this, TraceEffect, MuzzleLocation);
		if(TraceComponent)
		{
			TraceComponent->SetVectorParameter(TraceTargetName, TraceEnd);
		}
	}
}

void ASWeapon::DealPlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	if(IsProjectileWeapon())
	{
		//发射器类型武器的命中是延迟的，最好在发射物子弹里写命中特效
		return;
	}
	
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
		const FVector MuzzleLocation = MeshComponent->GetSocketLocation(MuzzleSocketName);

		//射击轨迹的向量
		FVector ShootDirection = ImpactPoint - MuzzleLocation;
		ShootDirection.Normalize();  //等同于Hit.ImpactNormal
		
		//击中特效
		UGameplayStatics::SpawnEmitterAtLocation(this, SelectedEffect, ImpactPoint,  ShootDirection.Rotation());

		//播放命中音效
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactPoint);
	}
}

void ASWeapon::PlayFireAnim_Implementation()
{
	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	if(MyOwner->GetIsAiming() && AimFireMontage)
	{
		CurrentFireMontage = AimFireMontage;
		MyOwner->PlayAnimMontage(AimFireMontage, AimPlayRate);
	}
	else if(NoAimFireMontage)
	{
		CurrentFireMontage = NoAimFireMontage;
		MyOwner->PlayAnimMontage(NoAimFireMontage, NoAimPlayRate);
	}
}

void ASWeapon::StopFireAnimAndTimer()//_Implementation()
{
	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	MyOwner->SetIsFiring(false);
	
	GetWorldTimerManager().ClearTimer(TimerHandle_TimerBetweenShot);
	MyOwner->StopAnimMontage(CurrentFireMontage);
}

void ASWeapon::StopReloadAnimAndTimer_Implementation()
{
	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	GetWorldTimerManager().ClearTimer(ReloadSoundTimer);
	MyOwner->StopAnimMontage(ReloadMontage);
}

void ASWeapon::StartFire()
{
	//第一次延迟时间
	float FirstDelay = LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds;
	FirstDelay = FMath::Clamp(FirstDelay, 0.f, FirstDelay);
	//FirstDelay = FMath::Max(FirstDelay, 0.f);

	//换弹时一直按着开火，换好后isFiring状态没有更新，所以最终挪到Fire()的可开火片段中设置
	//MyOwner->SetIsFiring(CheckCanFire());
	
	GetWorldTimerManager().SetTimer(TimerHandle_TimerBetweenShot, this, &ASWeapon::Fire, TimeBetweenShots, bIsFullAutomaticWeapon, FirstDelay);
}

void ASWeapon::StopFire()
{
	//停止播放射击动画
	//SetIsFiring(false)若放在Server端设置，
	//假设连射两发，打了一发多一点时间时停火，此时Client向Server发出停火指令，Server立刻停火，但是Multi函数有延迟，
	//可能会导致Timer因为延迟没有被立刻停止，使得Client多打了一发子弹并且IsFiring状态因此被设为true出现错误
	//所以收到StopFire()请求时，把SetIsFiring(false)和StopTimer放在一起执行防止延迟，并且Client和Server都立刻执行而不是Server通过Multi函数通知Client
	
	StopFireAnimAndTimer();
	
	if(!HasAuthority())
	{
		ServerStopFire();
		return;
	}
	
	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	if(CurrentAmmoNum == 0)
	{
		Reload(true);
	}
	
	//要处理啥，留个接口出来
	PostStopFire();
}

void ASWeapon::Reload(bool isAutoReload)
{
	if(!HasAuthority())
	{
		ServerReload(isAutoReload);
		return;
	}
	
	if(!CheckOwnerValidAndAlive() || MyOwner->GetIsReloading() || MyOwner->GetIsFiring() || CheckIsFullAmmo() || bIsCurrentAmmoInfinity)
	{
		return;
	}

	if(BackUpAmmoNum == 0)
	{
		//0备弹也广播，可能用于UI的备弹量数字颜色改变等小需求，C++服务器端不会自动调用OnRep，所以手动调用
		//如果当前子弹和备用子弹都空了，按开火后也会走到这里，调用了Reload(true)，不希望此时触发备用子弹的文字动画
		OnBackUpAmmoChanged.Broadcast(0, !isAutoReload);
		return;
	}
	
	MyOwner->SetIsReloading(true);

	const float MontagePlayTime = ReloadMontage && ReloadPlayRate>0.f ? ReloadMontage->SequenceLength/ReloadPlayRate : 1.0f ;

	if(ReloadMontage)
	{
		//Multi播放换弹动画，保证同步
		Multi_PlayReloadAnimAndSound();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("Reload蒙太奇不存在！")));
	}

	//GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("蒙太奇长度: %f"),MontagePlayTime));
	GetWorldTimerManager().SetTimer(ReloadTimer, [this](){StopReload(false);}, MontagePlayTime, false);

	//要处理啥，留个接口出来
	PostReload();
}

void ASWeapon::StopReload(bool IsInterrupted)
{
	if(!HasAuthority())
	{
		ServerStopReload(IsInterrupted);
		return;
	}
	
	if(!CheckOwnerValidAndAlive())
	{
		return;
	}
	
	MyOwner->SetIsReloading(false);
	StopReloadAnimAndTimer();
	
	if(!IsInterrupted)
	{
		if(CurrentAmmoNum < OnePackageAmmoNum)
		{
			const float TempR = FMath::Min(OnePackageAmmoNum-CurrentAmmoNum, OnceReloadAmmoNum);
			const float ReloadedNum = TempR > BackUpAmmoNum ? BackUpAmmoNum : TempR;
			BackUpAmmoNum = bIsBackUpAmmoInfinity ? BackUpAmmoNum : BackUpAmmoNum - ReloadedNum;
			CurrentAmmoNum = CurrentAmmoNum + ReloadedNum;
		}
		else if(CurrentAmmoNum == OnePackageAmmoNum && CanOverloadAmmo) //弹匣满了就只上一发子弹
		{
			BackUpAmmoNum = bIsBackUpAmmoInfinity ? BackUpAmmoNum : BackUpAmmoNum - 1;
			++CurrentAmmoNum;
		}
		else
		{
			return;
		}
		
		//刷新客户端当前子弹数
		SetClientCurrentAmmoNum(CurrentAmmoNum);

		if(HasAuthority())
		{
			//广播Reload事件，可用于更新子弹数UI等，C++服务器端手动调用一下子弹数更新委托
			OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, true);
			OnRep_BackUpAmmoNum();
		}

		//完成一次装弹后如果还没满且有弹药可装，则继续自动装弹(例如喷子一次上一两发，一直连续上弹)
		if(CurrentAmmoNum < OnePackageAmmoNum && BackUpAmmoNum > 0)
		{
			Reload(true);
		}
	}

	//要处理啥，留个接口出来
	PostStopReload();
}

void ASWeapon::ClientSyncCurrentAmmoOnFiring_Implementation(int ServerAmmo)
{
	if(!HasAuthority()) //ListenServer既是Server也是Client,需要加以限制
	{
		--AmmoSequence;
		CurrentAmmoNum = ServerAmmo-AmmoSequence;
	}
}

void ASWeapon::SetWeaponZoom_Implementation()
{
	if(CheckOwnerValidAndAlive())
	{
		if(!MyOwner->GetIsAiming()) //防止多次触发
		{
			DealWeaponZoom();
		}
	}
}

void ASWeapon::ResetWeaponZoom_Implementation()
{
	if(CheckOwnerValidAndAlive())
	{
		if(MyOwner->GetIsAiming()) //防止多次触发
		{
			DealWeaponResetZoom();
		}
	}
}

void ASWeapon::Multi_PlayReloadAnimAndSound_Implementation()
{
	if(CheckOwnerValidAndAlive() && ReloadMontage)
	{
		MyOwner->PlayAnimMontage(ReloadMontage, ReloadPlayRate);
		
		const float MontagePlayTime = ReloadMontage && ReloadPlayRate>0.f ? ReloadMontage->SequenceLength/ReloadPlayRate : 1.0f ;
		GetWorldTimerManager().SetTimer(ReloadSoundTimer,
		[this]()->void
		{
			if(ReloadSound)
			{
				WeaponSoundAudio->SetSound(ReloadSound);
				WeaponSoundAudio->Play();
			}
		},
		MontagePlayTime * 0.8f,
		false
			);
	}
}

void ASWeapon::ServerStopReload_Implementation(bool IsInterrupted)
{
	StopReload(IsInterrupted);
}

void ASWeapon::DealWeaponZoom()
{
	MyOwner->SetIsAiming(true);
}

void ASWeapon::DealWeaponResetZoom()
{
	MyOwner->SetIsAiming(false);
}

void ASWeapon::SetClientCurrentAmmoNum_Implementation(int ServerAmmo)
{
	OnCurrentAmmoChanged.Broadcast(ServerAmmo, ServerAmmo != CurrentAmmoNum);
	CurrentAmmoNum = ServerAmmo;
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

void ASWeapon::OnRep_WeaponPickUpInfo()
{
	//服务端在生成新武器时就执行了RefreshWeaponInfo，此时客户端还未完成复制，所以复制完成后手动刷新下武器Mesh
	GetWeaponMeshComp()->SetSkeletalMesh(WeaponPickUpInfo.WeaponMesh);
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

void ASWeapon::PostFire()
{
	//Do Something
}

void ASWeapon::PostStopFire()
{
	//Do Something
}

void ASWeapon::PostReload()
{
	//Do Something
}

void ASWeapon::PostStopReload()
{
	//Do Something
}

USkeletalMeshComponent* ASWeapon::GetWeaponMeshComp() const
{
	return MeshComponent;
}

//一个模板
void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	//DOREPLIFETIME_CONDITION(ASWeapon, CurrentAmmoNum, COND_None);
	DOREPLIFETIME_CONDITION(ASWeapon, BackUpAmmoNum, COND_None);
	DOREPLIFETIME(ASWeapon, bIsCurrentAmmoInfinity);
	DOREPLIFETIME(ASWeapon, bIsBackUpAmmoInfinity);
	DOREPLIFETIME(ASWeapon, WeaponName);
	DOREPLIFETIME(ASWeapon, WeaponPickUpInfo);
}


