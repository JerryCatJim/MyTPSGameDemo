// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FVector_NetQuantize TraceTo;

	//表面类型，将枚举转换为字节以正确赋值
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;
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

	int GetCurrentAmmoNum(){return CurrentAmmoNum;};

	UFUNCTION(BlueprintCallable)
	bool CheckCanFire();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool CheckOwnerValidAndAlive();
	
	//播放武器特效
	void PlayFireEffects(FVector TraceEnd);

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

	UFUNCTION(NetMulticast, Reliable)
	void Multi_PlayReloadMontage(UAnimMontage* MontageToPlay);
	
	//每当HitScanTrace这个变量被网络复制同步(在Fire函数中执行),就触发一次该函数
	UFUNCTION()  //绑定到委托的函数必须为void，且用UFUNCTION()进行标记
	void OnRep_HitScanTrace();

	UFUNCTION(BlueprintCallable)
	void OnRep_CurrentAmmoNum();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_BackUpAmmoNum();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_IsCurrentAmmoInfinity();
	
	UFUNCTION(BlueprintCallable)
	void OnRep_IsBackUpAmmoInfinity();

	//播放击中效果
	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	//播放射击动画
	UFUNCTION(NetMulticast, Reliable)
	void PlayFireAnim();
	UFUNCTION(NetMulticast, Reliable)
	//停止射击动画
	void StopFireAnim();
	
public:
	//当前子弹数
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_CurrentAmmoNum)
	int CurrentAmmoNum;
	//备用子弹数(不包括当前子弹数)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_BackUpAmmoNum)
	int BackUpAmmoNum;
	//是否无限当前子弹
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_IsCurrentAmmoInfinity)
	bool bIsCurrentAmmoInfinity = false;
	//是否无限备用子弹
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= "Weapon", ReplicatedUsing = OnRep_IsBackUpAmmoInfinity)
	bool bIsBackUpAmmoInfinity = false;

	//一个弹匣的装弹量
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	int OnePackageAmmoNum;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= "Weapon")
	FName WeaponName;

	//当前子弹数变化的委托
	UPROPERTY(BlueprintAssignable)
	FCurrentAmmoChangedDelegate OnCurrentAmmoChanged;

	//备用子弹数变化的委托
	UPROPERTY(BlueprintAssignable)
	FBackUpAmmoChangedDelegate OnBackUpAmmoChanged;

	//当前武器是否正在重新装填子弹
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category= "Weapon")
	bool bIsReloading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "WeaponMontage")
	UAnimMontage* ReloadMontage;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Component")
	class USkeletalMeshComponent* MeshComponent;

	//武器的持有者
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= "Weapon")
	class ASCharacter* MyOwner;
	
	//伤害类型子类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Weapon")
	class TSubclassOf<UDamageType> DamageType; //空值，传入后会选择默认类型

	//枪口插槽名称
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= "Weapon")
	FName MuzzleSocketName;

	//追踪目标名称
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Weapon")
	FName TraceTargetName;
	
	//枪口特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	UParticleSystem* MuzzleEffect;

	//默认击中特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	UParticleSystem* DefaultImpactEffect;

	//肉体击中效果(飙血)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	UParticleSystem* FleshImpactEffect;
	
	//易伤部位击中特效(例如爆头)
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	//UParticleSystem* HeadShotImpactEffect;
	
	//弹道特效
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
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

	//每次使用了该结构体进行网络复制，都会触发名为OnRep_HitScanTrace的函数
	UPROPERTY(ReplicatedUsing= OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	//装弹计时器
	FTimerHandle ReloadTimer;

	//瞄准时的开火动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponMontage)
	UAnimMontage* AimFireMontage;

	//腰射时(未瞄准)的开火动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponMontage)
	UAnimMontage* NoAimFireMontage;

	UPROPERTY()
	UAnimMontage* CurrentFireMontage;
	
	//开火动画的播放速率
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= WeaponMontage)
	float AimPlayRate = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= WeaponMontage)
	float NoAimPlayRate = 0.4f;
};
