// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapon/BaseWeapon/SWeapon.h"
#include "WeaponManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCurrentWeaponChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExchangeWeapon, FWeaponPickUpInfo, OldWeaponInfo);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPSGAME_API UWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponManagerComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetCurrentWeaponVisibility(bool IsVisible)
	{
		if(IsValid(CurrentWeapon))
		{
			CurrentWeapon->GetWeaponMeshComp()->bOwnerNoSee = !IsVisible;
			CurrentWeapon->bShowMuzzleFlash = IsVisible;
		}
	}
	void DestroyAllWeapons()
	{
		for(const auto& Type : WeaponEquipTypeList)
		{
			ASWeapon*& CurWeapon = GetWeaponByEquipType(Type);
			if(IsValid(CurWeapon) && CurWeapon->GetWeaponMeshComp())
			{
				CurWeapon->GetWeaponMeshComp()->SetVisibility(false);
				CurWeapon->ResetWeaponZoom();
			}
		}
	}
	
	//UFUNCTION(BlueprintCallable, Category = Weapon)
	void PickUpWeapon(FWeaponPickUpInfo WeaponInfo);

	//主动丢弃武器或者死亡时掉落武器,如果是手动丢弃则会施加扔出去的力，并将CurrentWeapon切换为下一把武器
	//UFUNCTION(BlueprintCallable, Category = Weapon)
	void StartDropWeapon(bool ManuallyDiscard);
	
	//控制武器开火
	//UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StartFire();
	//停止射击
	//UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StopFire();

	//武器重新装弹
	//UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StartReload();
	//UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StopReload();

	//将CurrentWeapon交换为指定类型的已装备的武器
	//UFUNCTION(BlueprintCallable, Category= WeaponSwap)
	void StartSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately = false);
	//UFUNCTION(BlueprintCallable, Category= WeaponSwap)
	void StopSwapWeapon(bool bWaitCurrentWeaponReplicated);

	//设置是否开镜
	//UFUNCTION(BlueprintCallable)  //将开镜行为发送到服务器然后同步
	void SetWeaponZoom();
	//UFUNCTION(BlueprintCallable)  //将开镜行为发送到服务器然后同步
	void ResetWeaponZoom();

	bool GetIsAiming() const { return bIsAiming; }
	void SetIsAiming(const bool& IsAiming){ bIsAiming = IsAiming; }

	bool GetIsFiring() const { return bIsFiring; }
	void SetIsFiring(const bool& IsFiring){ bIsFiring = IsFiring; }
	
	bool GetIsReloading()  const { return bIsReloading; }
	void SetIsReloading(const bool& IsReloading){ bIsReloading = IsReloading; }
	void SetIsReloadingLocally(const bool& IsReloading){ bIsReloadingLocally = IsReloading; }

	bool GetIsSwappingWeapon()  const { return bIsSwappingWeapon; }
	void SetIsSwappingWeapon(const bool& IsSwappingWeapon){ bIsSwappingWeapon = IsSwappingWeapon; }
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void SwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately);
	void LocalSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately);
	void DealPlaySwapWeaponAnim(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_ClientSyncPlaySwapWeaponAnim(EWeaponEquipType NewWeaponEquipType, bool Immediately);

	UFUNCTION(Server, Reliable)  //标记Server等RPC方法后不让TEnumAsByte做参数？(TEnumAsByte标记后，将值包装为了一个结构体)
	void ServerSwapWeapon(EWeaponEquipType NewWeaponEquipType, bool Immediately);

	//武器在服务器生成后复制到客户端有延迟，需要复制完成后再调用委托做一些初始化操作
	UFUNCTION()
	void OnRep_CurrentWeapon();
	/*UFUNCTION()
	void OnRep_MainWeapon();
	UFUNCTION()
	void OnRep_SecondaryWeapon();
	UFUNCTION()
	void OnRep_MeleeWeapon();
	UFUNCTION()
	void OnRep_ThrowableWeapon();*/

	UFUNCTION()//延迟较高时快速瞄准又退出时，本地会被服务器覆盖了旧时间的状态，将其改为本地最新状态
	void OnRep_IsAiming(){ if(IsLocallyControlled()) bIsAiming = bIsAimingLocally; }
	UFUNCTION()//原理同上
	void OnRep_IsFiring(){ if(IsLocallyControlled()) bIsFiring = bIsFiringLocally; }
	UFUNCTION()//原理同上
	void OnRep_IsReloading(){ if(IsLocallyControlled()) bIsReloading = bIsReloadingLocally; }
	UFUNCTION()//原理同上
	void OnRep_IsSwappingWeapon(){ if(IsLocallyControlled()) bIsSwappingWeapon = bIsSwappingWeaponLocally; }

	UFUNCTION(Server, Reliable)
	void DealDropWeapon(bool ManuallyDiscard);
	
	UFUNCTION(Server, Reliable)
	void DealPickUpWeapon(FWeaponPickUpInfo WeaponInfo);

private:
	ASWeapon* SpawnAndAttachWeapon(FWeaponPickUpInfo WeaponToSpawn, bool RefreshWeaponInfo = true);
	TSubclassOf<ASWeapon> GetWeaponSpawnClass(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	FName GetWeaponSocketName(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	ASWeapon*& GetWeaponByEquipType(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	UAnimMontage* GetSwapWeaponAnim(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	float GetSwapWeaponAnimRate(TEnumAsByte<EWeaponEquipType> WeaponEquipType);

	void DealSwapWeaponAttachment(TEnumAsByte<EWeaponEquipType> WeaponEquipType);

	void SwapToNextAvailableWeapon(TEnumAsByte<EWeaponEquipType> CurrentWeaponEquipType);

	bool IsLocallyControlled();
	
public:
	//当前武器
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, ReplicatedUsing = OnRep_CurrentWeapon)  //Replicated : 网络复制
	class ASWeapon* CurrentWeapon;

	//主武器
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, Replicated)  //Replicated : 网络复制
	ASWeapon* MainWeapon;
	//副武器 例如手枪
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, Replicated)  //Replicated : 网络复制
	ASWeapon* SecondaryWeapon;
	//近战武器 例如刀
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, Replicated)  //Replicated : 网络复制
	ASWeapon* MeleeWeapon;
	//投掷道具，例如手雷，烟雾弹
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, Replicated)  //Replicated : 网络复制
	ASWeapon* ThrowableWeapon;

	UPROPERTY(BlueprintAssignable)
	FCurrentWeaponChanged OnCurrentWeaponChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnExchangeWeapon OnExchangeWeapon;

	//防止同时与多个可拾取武器发生重叠时间
	bool bHasBeenOverlappedWithPickUpWeapon = false;
	
protected:
	UPROPERTY()
	class ASCharacter* MyOwnerPlayer;

	//是否正在开镜变焦
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsAiming, Category= Weapon)
	bool bIsAiming;
	//记录客户端本地的瞄准状态，以修正延迟较高时快速瞄准又退出时被服务器覆盖了旧时间的状态
	bool bIsAimingLocally;

	//是否正在射击
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsFiring, Category= Weapon)
	bool bIsFiring;
	//记录客户端本地的射击状态，防止延迟较高时，本地预测的先行状态被服务器覆盖了旧时间的状态
	bool bIsFiringLocally;
	
	//当前武器是否正在重新装填子弹
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsReloading, Category= Weapon)
	bool bIsReloading = false;
	//记录客户端本地的装弹状态，防止延迟较高时，本地预测的先行状态被服务器覆盖了旧时间的状态
	bool bIsReloadingLocally;

	//当前武器是否正在交换武器
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsSwappingWeapon, Category= Weapon)
	bool bIsSwappingWeapon = false;
	//记录客户端本地的交换武器状态，防止延迟较高时，本地预测的先行状态被服务器覆盖了旧时间的状态
	bool bIsSwappingWeaponLocally;

	//为玩家生成武器
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<ASWeapon> MainWeaponClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<ASWeapon> SecondaryWeaponClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<ASWeapon> MeleeWeaponClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<ASWeapon> ThrowableWeaponClass;

	//武器插槽名称
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	FName CurrentWeaponSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	FName MainWeaponSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	FName SecondaryWeaponSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	FName MeleeWeaponSocketName;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	FName ThrowableWeaponSocketName;

	//角色切换武器时的动画
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SwapWeapon)
	UAnimMontage* SwapMainWeaponAnim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SwapWeapon)
	UAnimMontage* SwapSecondaryWeaponAnim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SwapWeapon)
	UAnimMontage* SwapMeleeWeaponAnim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = SwapWeapon)
	UAnimMontage* SwapThrowableWeaponAnim;
	UPROPERTY()
	UAnimMontage* CurrentSwapWeaponAnim; //记录当前正在播放的切换动画，如果切换过程中又想切换回原武器，则反播此动画即可
	
	//切换武器动画的播放速率
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= SwapWeapon)
	float SwapMainWeaponRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= SwapWeapon)
	float SwapSecondaryWeaponRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= SwapWeapon)
	float SwapMeleeWeaponRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category= SwapWeapon)
	float SwapThrowableWeaponRate = 1.f;
	//切换武器动画计时器
	FTimerHandle SwapWeaponTimer;

private:
	UPROPERTY()
	TArray<TEnumAsByte<EWeaponEquipType>> WeaponEquipTypeList = {
		EWeaponEquipType::MainWeapon,
		EWeaponEquipType::SecondaryWeapon,
		EWeaponEquipType::MeleeWeapon,
		EWeaponEquipType::ThrowableWeapon
	};
	
};
