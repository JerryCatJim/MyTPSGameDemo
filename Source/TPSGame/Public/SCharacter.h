// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/BaseWeapon/SWeapon.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Component/SBuffComponent.h"
#include "Component/SHealthComponent.h"
#include "Component/InventoryComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interface/MyInterfaceTest.h"
#include "TPSGameType/Team.h"
#include "SCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCurrentWeaponChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyDown);  //对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件，用以区分
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyLongPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerDead, AController*, InstigatedBy, AActor*, DamageCauser,const UDamageType*, DamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExchangeWeapon, FWeaponPickUpInfo, OldWeaponInfo);

UCLASS()
class TPSGAME_API ASCharacter : public ACharacter, public IMyInterfaceTest
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//重写父类函数,在生命周期中进行网络复制
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//角色被销毁时调用
	virtual void Destroyed() override;

	//通常只在服务端触发, 刚进游戏时服务端先OnPossess然后BeginPlay，RestartPlayer后服务端先BeginPlay然后OnPossess
	virtual void PossessedBy(AController* NewController) override;
	
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void PickUpWeapon(FWeaponPickUpInfo WeaponInfo);

	//主动丢弃武器或者死亡时掉落武器,如果是手动丢弃则会施加扔出去的力，并将CurrentWeapon切换为下一把武器
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void StartDropWeapon(bool ManuallyDiscard);
	
	//控制武器开火
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StartFire();
	//停止射击
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StopFire();

	//武器重新装弹
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StartReload();
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StopReload();

	//将CurrentWeapon交换为指定类型的已装备的武器
	UFUNCTION(BlueprintCallable, Category= WeaponSwap)
	void StartSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately = false);
	UFUNCTION(BlueprintCallable, Category= WeaponSwap)
	void StopSwapWeapon(bool bWaitCurrentWeaponReplicated);

	//设置是否开镜
	UFUNCTION(BlueprintCallable)  //将开镜行为发送到服务器然后同步
	void SetWeaponZoom();
	UFUNCTION(BlueprintCallable)  //将开镜行为发送到服务器然后同步
	void ResetWeaponZoom();

	//当交互键(E)被按下时
	UFUNCTION(BlueprintCallable)//, Server, Reliable)
	void InteractKeyPressed();
	UFUNCTION(BlueprintCallable)//, Server, Reliable)
	void InteractKeyReleased();
	UFUNCTION(BlueprintCallable)//, Server, Reliable)
	void TryLongPressInteractKey();   //尝试长按交互键
	void BeginLongPressInteractKey(); //进入长按状态
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_OnInteractKeyDown();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_OnInteractKeyUp();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_OnInteractKeyLongPressed();
	
	//重写,获取摄像机组件位置
	virtual FVector GetPawnViewLocation() const override;

	UCameraComponent* GetCameraComponent()  const { return CameraComponent; }
	
	USHealthComponent* GetHealthComponent() const { return HealthComponent; }
	USBuffComponent* GetBuffComponent()     const { return BuffComponent; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UInventoryComponent* GetInventoryComponent();
	
	bool GetIsDied() const { return bDied; }
	ETeam GetTeam() const { return PlayerTeam; }
	
	bool GetIsAiming() const { return bIsAiming; }
	void SetIsAiming(const bool& IsAiming){ bIsAiming = IsAiming; }

	bool GetIsFiring() const { return bIsFiring; }
	void SetIsFiring(const bool& IsFiring){ bIsFiring = IsFiring; }
	
	bool GetIsReloading()  const { return bIsReloading; }
	void SetIsReloading(const bool& IsReloading){ bIsReloading = IsReloading; }
	void SetIsReloadingLocally(const bool& IsReloading){ bIsReloadingLocally = IsReloading; }

	bool GetIsSwappingWeapon()  const { return bIsSwappingWeapon; }
	void SetIsSwappingWeapon(const bool& IsSwappingWeapon){ bIsSwappingWeapon = IsSwappingWeapon; }
	
	float GetAimOffset_Y() const { return AimOffset_Y; }
	float GetAimOffset_Z() const { return AimOffset_Z; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetSpringArmLength(){ return SpringArmComponent ? SpringArmComponent->TargetArmLength : 0 ; }
	
	UFUNCTION()
	void OnMatchEnd(int NewWinnerID, ETeam NewWinningTeam);
	
	UFUNCTION(Server, Reliable)
	void PlayerLeaveGame();

	UFUNCTION(BlueprintCallable)
	void SetBodyColor(ETeam Team);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void LookUp(float Value);
	void Turn(float Value);
	
	void MoveForward(float Value);
	void MoveRight(float Value);

	void BeginJump();
	void EndJump();
	
	void BeginCrouch();
	void EndCrouch();

	void SwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately);
	void LocalSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately);
	void DealPlaySwapWeaponAnim(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately);
	UFUNCTION(NetMulticast, Reliable)
	void Multi_ClientSyncPlaySwapWeaponAnim(EWeaponEquipType NewWeaponEquipType, bool Immediately);

	UFUNCTION(Server, Reliable)  //标记Server等RPC方法后不让TEnumAsByte做参数？(TEnumAsByte标记后，将值包装为了一个结构体)
	void ServerSwapWeapon(EWeaponEquipType NewWeaponEquipType, bool Immediately);
	
	//根据是否开镜设置人物的移动类型
	UFUNCTION(BlueprintNativeEvent)
	void SetPlayerControllerRotation();

	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void SyncAimOffset();  //放在了LookUp()中调用
	
	//生命值更改函数
	UFUNCTION()//Server, Reliable)  //必须UFUNCTION才能绑定委托
	void OnHealthChanged(class USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, //HealthDelta 生命值改变量,增加或减少
	const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

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

	//角色死亡后做的一些操作
	UFUNCTION()
	void OnRep_Died();

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
	void HideCharacterIfCameraClose();
	void TryInitBodyColor();
	void LoopSetBodyColor();

	UFUNCTION()
	void OnRep_PlayerTeam(){ SetBodyColor(PlayerTeam); }

	ASWeapon* SpawnAndAttachWeapon(FWeaponPickUpInfo WeaponToSpawn, bool RefreshWeaponInfo = true);
	TSubclassOf<ASWeapon> GetWeaponSpawnClass(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	FName GetWeaponSocketName(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	ASWeapon*& GetWeaponByEquipType(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	UAnimMontage* GetSwapWeaponAnim(TEnumAsByte<EWeaponEquipType> WeaponEquipType);
	float GetSwapWeaponAnimRate(TEnumAsByte<EWeaponEquipType> WeaponEquipType);

	void DealSwapWeaponAttachment(TEnumAsByte<EWeaponEquipType> WeaponEquipType);

	void SwapToNextAvailableWeapon(TEnumAsByte<EWeaponEquipType> CurrentWeaponEquipType);
	
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

	UPROPERTY(EditAnywhere)
	float DistanceToHideCharacter = 100;

	//禁止除了旋转镜头以外的游戏操作(例如移动，开火等)输入
	UPROPERTY(Replicated)
	bool bDisableGamePlayInput = false;

	UPROPERTY(BlueprintAssignable)
	FCurrentWeaponChanged OnCurrentWeaponChanged;

	UPROPERTY(BlueprintAssignable)
	FPlayerDead OnPlayerDead;

	UPROPERTY(BlueprintAssignable)
	FOnExchangeWeapon OnExchangeWeapon;

	UPROPERTY(BlueprintAssignable)  //对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件
	FOnInteractKeyUp OnInteractKeyDown;
	UPROPERTY(BlueprintAssignable)
	FOnInteractKeyUp OnInteractKeyUp;
	UPROPERTY(BlueprintAssignable)
	FOnInteractKeyLongPressed OnInteractKeyLongPressed;

	//防止同时与多个可拾取武器发生重叠时间
	bool bHasBeenOverlappedWithPickUpWeapon = false;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USpringArmComponent* SpringArmComponent;
	
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
	
	//默认视野范围
	float DefaultFOV;

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

	//生命值组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USHealthComponent* HealthComponent;

	//Buff组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USBuffComponent* BuffComponent;

	//角色是否死亡
	UPROPERTY(ReplicatedUsing = OnRep_Died, BlueprintReadOnly, Category= PlayerStatus)
	bool bDied;

	//瞄准偏移量
	UPROPERTY(Replicated, BlueprintReadWrite, Category= PlayerStatus)
	float AimOffset_Y;

	UPROPERTY(Replicated, BlueprintReadWrite, Category= PlayerStatus)
	float AimOffset_Z;

	//角色重生倒计时
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	float RespawnCount = 5;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PlayerTeam, Category=Team)
	ETeam PlayerTeam = ETeam::ET_NoTeam;
	
	UPROPERTY(EditAnywhere, Category=PlayerColor)
	UMaterialInstance* OriginalMaterial;
	UPROPERTY(EditAnywhere, Category=PlayerColor)
	UMaterialInstance* RedMaterial;
	UPROPERTY(EditAnywhere, Category=PlayerColor)
	UMaterialInstance* BlueMaterial;

	//按X秒互动键(E)判定进入长按的计时器
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PlayerTimer)
	FTimerHandle FInteractKeyLongPressBeginHandle;
	//按X秒进入长按
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	float InteractKeyLongPressBeginSecond = 0.5f;
	//是否进入长按状态
	bool bIsLongPressing;
	
	//长按X秒互动键(E)完成长按的计时器
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = PlayerTimer)
	FTimerHandle FInteractKeyLongPressFinishHandle;
	//按X秒完成长按
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	float InteractKeyLongPressFinishSecond = 2.f;

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
	bool bPlayerLeftGame = false;
	
	FTimerHandle FGetPlayerStateHandle;
	int TryGetPlayerStateTimes = 0;  //超过5次还没成功就停止计时器

	UPROPERTY()
	TArray<TEnumAsByte<EWeaponEquipType>> WeaponEquipTypeList = {
		EWeaponEquipType::MainWeapon,
		EWeaponEquipType::SecondaryWeapon,
		EWeaponEquipType::MeleeWeapon,
		EWeaponEquipType::ThrowableWeapon
	};
	
//临时测试接口的区域
public:
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = TestInterface)
	//bool NotEvent_NativeTest();
	virtual bool NotEvent_NativeTest_Implementation() override;
	
	//virtual void IsEvent_NativeTest_Implementation() override;
	
};
