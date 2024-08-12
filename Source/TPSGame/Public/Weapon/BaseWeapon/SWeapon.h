// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

UENUM(BlueprintType)
enum EWeaponBulletType
{
	HitScan,
	Projectile,
};

UENUM(BlueprintType)
enum EWeaponType
{
	Rifle,
	Pistol,
	RocketLauncher,
	ShotGun,
	MachineGun,
};

USTRUCT(BlueprintType)
struct FWeaponPickUpInfo
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class ASCharacter* Owner;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ASWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int CurrentAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int BackUpAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeaponName;

	FWeaponPickUpInfo(){};
	
	FWeaponPickUpInfo(ASCharacter* NewOwner, USkeletalMesh* NewWeaponMesh, TSubclassOf<ASWeapon> NewWeaponClass, int NewCurrentAmmo, int NewBackUpAmmo, FName NewWeaponName)
	{
		Owner = NewOwner;
		WeaponMesh = NewWeaponMesh;
		WeaponClass = NewWeaponClass;
		CurrentAmmo = NewCurrentAmmo;
		BackUpAmmo = NewBackUpAmmo;
		WeaponName = NewWeaponName;
	};
};

//IsEmpty用以区分当次射击是否为空射(可用于播放子弹数为空时的音效等)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentAmmoChangedDelegate, int, CurrentAmmoNum, bool, bPlayEffect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBackUpAmmoChangedDelegate, int, BackUpAmmoNum, bool, bPlayEffect);

UCLASS()
class TPSGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	//重写父类函数,在生命周期中进行网络复制
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	USkeletalMeshComponent* GetWeaponMeshComp() const;
	
	//开始射击函数
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void StartFire();
	
	//停止射击函数
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void StopFire();

	//重新装填子弹函数
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Reload(bool isAutoReload = false);

	UFUNCTION()
	void StopReload(bool IsInterrupted = false);

	int GetCurrentAmmoNum() const { return CurrentAmmoNum; }

	TSubclassOf<UDamageType> GetWeaponDamageType() const { return DamageType; }

	EWeaponType GetWeaponType() const { return WeaponType; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetCurrentAimingPoint(bool bUseSpread = true);
	
	UFUNCTION(BlueprintCallable)
	bool CheckCanFire();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Weapon)
	FWeaponPickUpInfo GetWeaponPickUpInfo();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void RefreshWeaponInfo(FWeaponPickUpInfo WeaponInfo);

	float GetHeadShotBonus() const { return HeadShotBonus; }
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool CheckOwnerValidAndAlive();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsProjectileWeapon();
	
	//计算武器射击扩散程度
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual float GetDynamicBulletSpread();

	//装弹是否已满
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CheckIsFullAmmo();
	
	//处理射击判定的函数
	virtual void DealFire();
	
	//武器射击函数
	UFUNCTION(BlueprintCallable, Category= "Weapon")
	void Fire();

	//Server服务器端开火函数(客户端client发出请求到服务器执行)
	UFUNCTION(Server, Reliable, WithValidation) //服务器，可靠链接，进行验证 （RPC函数）
	void ServerFire();

	UFUNCTION(BlueprintNativeEvent)
	void ServerFire_BP();

	UFUNCTION(Server, Reliable) //服务器，可靠链接
	void ServerStopFire();

	UFUNCTION(BlueprintNativeEvent)
	void ServerStopFire_BP();

	UFUNCTION(Server, Reliable) //服务器，可靠链接，进行验证 （RPC函数）
	void ServerReload(bool isAutoReload);

	UFUNCTION(BlueprintNativeEvent)
	void ServerReload_BP();

	UFUNCTION(Server, Reliable) //服务器，可靠链接，进行验证 （RPC函数）
	void ServerStopReload(bool IsInterrupted = false);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayReloadMontageAndSound();

	UFUNCTION(BlueprintCallable)
	void OnRep_CurrentAmmoNum();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_BackUpAmmoNum();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_IsCurrentAmmoInfinity();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_IsBackUpAmmoInfinity();

	UFUNCTION(BlueprintCallable)
	void OnRep_WeaponPickUpInfo();

	//播放武器开火特效
	UFUNCTION(NetMulticast, Unreliable)
	void PlayFireEffectsAndSounds();

	UFUNCTION(NetMulticast, Unreliable)
	virtual void PlayTraceEffect(FVector TraceEnd);  //子弹轨迹特效
	UFUNCTION(NetMulticast, Unreliable)
	virtual void PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector ImpactPoint);  //命中特效和声音

	//播放射击动画
	UFUNCTION(NetMulticast, Reliable)
	void PlayFireAnim();
	//停止射击动画
	UFUNCTION()//NetMulticast, Reliable)
	void StopFireAnimAndTimer();
	//停止装弹动画和计时器
	UFUNCTION(NetMulticast, Reliable)
	void StopReloadAnimAndTimer();

	//获取开枪起始位置
	FVector GetWeaponShootStartPoint(FVector EyeLocation, FRotator EyeRotation);
	
public:
	//当前子弹数
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_CurrentAmmoNum)
	int CurrentAmmoNum;
	//备用子弹数(不包括当前子弹数)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_BackUpAmmoNum)
	int BackUpAmmoNum;
	//一个弹匣的装弹量
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	int OnePackageAmmoNum;
	//一次装弹的弹药量(默认为每个弹匣的容量)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	int OnceReloadAmmoNum;
	//是否满装弹后可以再装填一发
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	bool CanOverloadAmmo = true;
	
	//是否无限当前子弹
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_IsCurrentAmmoInfinity)
	bool bIsCurrentAmmoInfinity = false;
	//是否无限备用子弹
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_IsBackUpAmmoInfinity)
	bool bIsBackUpAmmoInfinity = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon", Replicated)
	FName WeaponName;
	
	//开镜后的视野范围
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponZoom)
	float ZoomedFOV;
	//开镜速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponZoom, meta=(ClampMin = 0.1f, ClampMax = 100.f))
	float ZoomInterpSpeed;
	
	//当前子弹数变化的委托
	UPROPERTY(BlueprintAssignable)
	FCurrentAmmoChangedDelegate OnCurrentAmmoChanged;

	//备用子弹数变化的委托
	UPROPERTY(BlueprintAssignable)
	FBackUpAmmoChangedDelegate OnBackUpAmmoChanged;

	//当前武器是否正在重新装填子弹
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category= "Weapon")
	bool bIsReloading = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	bool bIsFullAutomaticWeapon = true;

	//SCharacter中因人物与摄像机过近时而隐藏人物和武器时，开枪也不显示枪口火焰特效
	bool bShowMuzzleFlash = true;
	
	//武器的准星类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Weapon")
	TSubclassOf<class UTPSCrossHair> CrossHairClass;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TEnumAsByte<EWeaponType> WeaponType = EWeaponType::Rifle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Component")
	class USkeletalMeshComponent* MeshComponent;

	UPROPERTY(VisibleAnywhere, Category=Component)
	UAudioComponent* WeaponSoundAudio;
	
	//武器的持有者
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= "Weapon")
	class ASCharacter* MyOwner;
	
	//伤害类型子类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Weapon")
	TSubclassOf<UDamageType> DamageType; //空值，传入后会选择默认类型
	
	//枪口插槽名称
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Weapon")
	FName MuzzleSocketName;

	//要设置长度的粒子特效的变量名称(?)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Weapon")
	FName TraceTargetName;
	
	//枪口特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponEffect)
	UParticleSystem* MuzzleEffect;

	//默认击中特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponEffect)
	UParticleSystem* DefaultImpactEffect;
	//肉体击中效果(飙血)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponEffect)
	UParticleSystem* FleshImpactEffect;
	//易伤部位击中特效(例如爆头)
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponEffect)
	//UParticleSystem* HeadShotImpactEffect;

	//开火音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponSound)
	class USoundCue* FireSound;
	//默认击中音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponSound)
	class USoundCue* DefaultHitSound;
	//肉体击中音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponSound)
	class USoundCue* FleshHitSound;
	//易伤部位击中音效(例如爆头)
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponSound)
	//class USoundCue* HeadShotHitSound;
	//换弹音效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponSound)
	class USoundCue* ReloadSound;
	
	//弹道特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponEffect)
	UParticleSystem* TraceEffect;

	//摄像机抖动子类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<UCameraShakeBase> FireCameraShake;
	
	//射击基础伤害
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	float BaseDamage;

	//爆头伤害倍率
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	float HeadShotBonus;

	//武器最远射程
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Weapon)
	float WeaponTraceRange = 10000;

	//射击命中的位置
	FVector ShotTraceEnd;
	//射击时命中的物理表面类型
	EPhysicalSurface HitSurfaceType = SurfaceType_Default;
	
	//连射间隔_计时器句柄
	FTimerHandle TimerHandle_TimerBetweenShot;

	//上次开火时间
	float LastFireTime;

	//子弹扩散程度(数值会被转化为弧度,作为圆锥轴与其侧边的夹角弧度)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon, meta=(ClampMin = 0.f))
	float BulletSpread;
	
	//连续射击速度(每分钟射速,不是两发之间的秒数间隔)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category= Weapon, meta=(ClampMin = 100.f, ClampMax = 900.f))
	float RateOfShoot;

	//射击间隔(60除以射击速度)
	float TimeBetweenShots;

	//装弹计时器
	FTimerHandle ReloadTimer;
	//播放装弹声音计时器
	FTimerHandle ReloadSoundTimer;

	//瞄准时的开火动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponMontage)
	UAnimMontage* AimFireMontage;

	//腰射时(未瞄准)的开火动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponMontage)
	UAnimMontage* NoAimFireMontage;

	//装弹动画
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "WeaponMontage")
	UAnimMontage* ReloadMontage;
	
	UPROPERTY()
	UAnimMontage* CurrentFireMontage;
	
	//开火动画的播放速率
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= WeaponMontage)
	float AimPlayRate = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= WeaponMontage)
	float NoAimPlayRate = 0.4f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= WeaponMontage)
	float ReloadPlayRate = 1.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	TEnumAsByte<EWeaponBulletType> WeaponBulletType;

private:
	UPROPERTY(ReplicatedUsing = OnRep_WeaponPickUpInfo)
	FWeaponPickUpInfo WeaponPickUpInfo;
};
