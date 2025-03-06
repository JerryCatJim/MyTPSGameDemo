// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponAndBulletType/WeaponType.h"
#include "WeaponAndBulletType/BulletType.h"
#include "SWeapon.generated.h"

class APickUpWeapon;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EWeaponEquipType> WeaponEquipType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsWeaponValid;

	FWeaponPickUpInfo(){IsWeaponValid = false;};
	
	FWeaponPickUpInfo(ASCharacter* NewOwner, USkeletalMesh* NewWeaponMesh, TSubclassOf<ASWeapon> NewWeaponClass, int NewCurrentAmmo, int NewBackUpAmmo, FName NewWeaponName,
		TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool IsValidWeapon = true)
	{
		Owner = NewOwner;
		WeaponMesh = NewWeaponMesh;
		WeaponClass = NewWeaponClass;
		CurrentAmmo = NewCurrentAmmo;
		BackUpAmmo = NewBackUpAmmo;
		WeaponName = NewWeaponName;
		WeaponEquipType = NewWeaponEquipType;
		IsWeaponValid = IsValidWeapon;
	};
};

//IsEmpty用以区分当次射击是否为空射(可用于播放子弹数为空时的音效等)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentAmmoChangedDelegate, int, CurrentAmmoNum, bool, bPlayEffect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBackUpAmmoChangedDelegate, int, BackUpAmmoNum, bool, bPlayEffect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHitTargetDelegate, bool, bIsEnemy, bool, bIsHeadshot);

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
	UFUNCTION(BlueprintCallable)
	void StartFire();
	//停止射击函数
	UFUNCTION(BlueprintCallable)
	void StopFire();

	//重新装填子弹函数
	UFUNCTION(BlueprintCallable)
	void StartReload();
	UFUNCTION(BlueprintCallable)
	void StopReload();
	
	//设置武器是否开镜
	UFUNCTION(BlueprintCallable)  //将开镜行为发送到服务器然后同步
	bool SetWeaponZoom();
	UFUNCTION(BlueprintCallable)  //将开镜行为发送到服务器然后同步
	bool ResetWeaponZoom();
	
	int GetCurrentAmmoNum() const { return CurrentAmmoNum; }

	TSubclassOf<UDamageType> GetWeaponDamageType() const { return DamageType; }

	UFUNCTION(BlueprintCallable)
	TSubclassOf<APickUpWeapon> GetPickUpWeaponClass() const { return PickUpWeaponClass; }
	
	EWeaponType GetWeaponType() const { return WeaponType; }
	EWeaponEquipType GetWeaponEquipType() const { return WeaponEquipType; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetCurrentAimingPoint(bool bUseSpread = true);
	
	UFUNCTION(BlueprintCallable)
	bool CheckCanFire();
	UFUNCTION(BlueprintCallable)
	bool CheckCanReload();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Weapon)
	FWeaponPickUpInfo GetWeaponPickUpInfo();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void RefreshWeaponInfo(FWeaponPickUpInfo WeaponInfo);

	float GetHeadShotBonus() const { return HeadShotBonus; }

	UFUNCTION(BlueprintCallable, Client, Reliable)  //直接改变子弹数，不修正AmmoSequence
	void ClientChangeCurrentAmmo(int ChangedNum);  //减少则ChangedNum填入负数，增加则填入正数

	UFUNCTION(BlueprintCallable)
	bool CheckIsMeleeWeapon() const { return GetWeaponType() == EWeaponType::Knife || GetWeaponType() == EWeaponType::Fist; }

	UFUNCTION(BlueprintCallable)
	bool GetWeaponCanDropDown() const { return bCanDropDown; }
	UFUNCTION(BlueprintCallable)
	void SetWeaponCanDropDown(bool NewCanDropDown) { bCanDropDown = NewCanDropDown; }
	UFUNCTION(BlueprintCallable)
	bool GetWeaponCanManuallyDiscard() const { return bCanManuallyDiscard; }
	UFUNCTION(BlueprintCallable)
	void SetWeaponCanManuallyDiscard(bool NewCanManuallyDiscard) { bCanManuallyDiscard = NewCanManuallyDiscard; }
	
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
	void Fire();
	void LocalFire();

	//本地停止射击函数
	void LocalStopFire();

	//装弹函数
	void Reload(bool IsAutoReload = false);
	void LocalReload(bool IsAutoReload = false);
	void ReloadFinished();
	void LocalReloadFinished();

	//本地停止装弹函数
	void LocalStopReload();

	//Server服务器端开火函数(客户端client发出请求到服务器执行)
	UFUNCTION(Server, Reliable, WithValidation) //服务器，可靠链接，进行验证 （RPC函数）
	void ServerFire();
	UFUNCTION(Server, Reliable) //服务器，可靠链接
	void ServerStopFire();
	
	UFUNCTION(Server, Reliable) //服务器，可靠链接，进行验证 （RPC函数）
	void ServerReload(bool isAutoReload);
	UFUNCTION(Server, Reliable) //服务器，可靠链接，进行验证 （RPC函数）
	void ServerStopReload();
	
	UFUNCTION(BlueprintCallable)
	virtual void PostFire();
	UFUNCTION(BlueprintCallable)
	virtual void PostStopFire();
	UFUNCTION(BlueprintCallable)
	virtual void PostReload();
	UFUNCTION(BlueprintCallable)
	virtual void PostReloadFinished();

	//将不同枪在开镜时的不同需求剥离出来一个函数单独处理，而且不要标记Server,以便客户端高延迟时也能直接响应
	virtual void PreDealWeaponZoom();
	virtual void PreDealWeaponResetZoom();
	UFUNCTION(Server,Reliable)
	void DealWeaponZoom();
	UFUNCTION(Server,Reliable)
	void DealWeaponResetZoom();
	
	//UFUNCTION(NetMulticast, Reliable)
	void PlayReloadAnimAndSound();
	UFUNCTION(NetMulticast, Reliable)
	void Multi_ClientSyncPlayReloadAnimAndSound();
	
	UFUNCTION(Client, Reliable)  //只作为验证函数，若想直接改变数值则调用ClientChangeCurrentAmmo()
	void ClientSyncCurrentAmmo(int ServerAmmo, int ChangedNum);  //减少则ChangedNum填入负数，增加则填入正数
	
	UFUNCTION(BlueprintCallable)
	void UpdateCurrentAmmoChange(bool PlayEffect){ OnCurrentAmmoChanged.Broadcast(CurrentAmmoNum, PlayEffect); }
	
	UFUNCTION(BlueprintCallable)
	void OnRep_BackUpAmmoNum();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_IsCurrentAmmoInfinity();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_IsBackUpAmmoInfinity();

	UFUNCTION(BlueprintCallable)
	void OnRep_WeaponPickUpInfo();

	UFUNCTION(NetMulticast, Reliable)
	void PlayTraceEffect(FVector TraceEnd);  //子弹轨迹特效
	UFUNCTION(NetMulticast, Reliable)
	void PlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector ImpactPoint);  //命中特效和声音
	virtual void DealPlayTraceEffect(FVector TraceEnd);  //不要重写Multi函数，会导致客户端多次触发，把要重写的部分抽象出来单做一个函数
	virtual void DealPlayImpactEffectsAndSounds(EPhysicalSurface SurfaceType, FVector ImpactPoint);
	
	//播放武器开火特效
	//UFUNCTION(NetMulticast, Unreliable)
	void PlayFireEffectsAndSounds();
	//播放射击动画
	//UFUNCTION(NetMulticast, Reliable)
	void PlayFireAnim();
	//停止射击动画
	//UFUNCTION(NetMulticast, Reliable)
	void StopFireAnimAndTimer();
	//停止装弹动画和计时器
	//UFUNCTION(NetMulticast, Reliable)
	void StopReloadAnimAndTimer();

	UFUNCTION(NetMulticast, Reliable)
	void Multi_ClientSyncFireAnimAndEffectsAndSounds();

	//获取开枪起始位置
	FVector GetWeaponShootStartPoint();

private:
	UFUNCTION(BlueprintCallable)
	FVector GetEnemyPositionNearestToCrossHair();
	
	UFUNCTION(BlueprintCallable)
	bool IsInScreenViewport(const FVector& WorldPosition);
	
public:
	//当前子弹数
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	int CurrentAmmoNum;
	//本地相较于服务器未经同步验证的子弹数差值(负数为本地比服务器少的值，正数为本地比服务器多的值)
	int AmmoSequence = 0;  //射击次数权威仍在服务端，但是本地端预测了开火特效和枪声，延迟过大时可能出现开枪声音次数大于实际次数的问题，用此变量来记录因延迟未被同步的子弹数差值
	
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

	//是否自动瞄准锁定离准星最近敌人
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Weapon")//, ReplicatedUsing = OnRep_IsCurrentAmmoInfinity)
	bool bIsAutoLockEnemy = false;
	//自动索敌时的最远距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Weapon")//, ReplicatedUsing = OnRep_IsCurrentAmmoInfinity)
	float AutoLockEnemyDistanceMax = 3000;
	//自动索敌时的最高高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Weapon")//, ReplicatedUsing = OnRep_IsCurrentAmmoInfinity)
	float AutoLockEnemyHeightMax = 500;
	
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

	UPROPERTY(BlueprintAssignable)
	FHitTargetDelegate OnWeaponHitTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	bool bIsFullAutomaticWeapon = true;

	//SCharacter中因人物与摄像机过近时而隐藏人物和武器时，开枪也不显示枪口火焰特效
	bool bShowMuzzleFlash = true;
	
	//武器的准星类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Weapon")
	TSubclassOf<class UTPSCrossHair> CrossHairClass;
	//武器的命中反馈类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Weapon")
	TSubclassOf<UUserWidget> HitFeedbackCrossHairClass;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TEnumAsByte<EWeaponType> WeaponType = EWeaponType::Rifle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Weapon)
	TEnumAsByte<EWeaponEquipType> WeaponEquipType;
	
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

	UPROPERTY(BlueprintReadWrite, Category= Weapon, Replicated)
	FVector EyeLocation_Rep;
	UPROPERTY(BlueprintReadWrite, Category= Weapon, Replicated)
	FRotator EyeRotation_Rep;
	
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category= Weapon, meta=(ClampMin = 1.f, ClampMax = 9999.f))
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

	//武器是否可以死后掉落
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Weapon, Replicated)
	bool bCanDropDown = true;
	//武器是否可以手动丢弃
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= Weapon, Replicated)
	bool bCanManuallyDiscard = true;
	//不填则在C++文件的BeginPlay中默认选择BP_PickUpWeaponBase
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<APickUpWeapon> PickUpWeaponClass;
	
private:
	UPROPERTY(ReplicatedUsing = OnRep_WeaponPickUpInfo)
	FWeaponPickUpInfo WeaponPickUpInfo;
};
