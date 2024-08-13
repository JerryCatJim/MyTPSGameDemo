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
#include "TPSPlayerController.h"
#include "Interface/MyInterfaceTest.h"
#include "SCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCurrentWeaponChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyDown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerDead, AController*, InstigatedBy, AActor*, DamageCauser,const UDamageType*, DamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUpWeapon, FWeaponPickUpInfo, OldWeaponInfo);

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
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Weapon)
	void PickUpWeapon(FWeaponPickUpInfo WeaponInfo);

	//死亡时掉落武器
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Weapon)
	void DropWeapon();
	
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
	
	UFUNCTION(Server,Reliable)  //将射击行为发送到服务器然后同步
	void SetIsFiring(bool IsFiring);
	
	//重写,获取摄像机组件位置
	virtual FVector GetPawnViewLocation() const override;

	UCameraComponent* GetCameraComponent()  const { return CameraComponent; }
	
	USHealthComponent* GetHealthComponent() const { return HealthComponent; }
	USBuffComponent* GetBuffComponent()     const { return BuffComponent; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UInventoryComponent* GetInventoryComponent();
	
	bool GetIsDied()   const { return bDied; }

	bool GetIsAiming() const { return bIsAiming; }
	void SetIsAiming(const bool& IsAiming){ bIsAiming = IsAiming; }

	bool GetIsFiring() const { return bIsFiring; }
	
	bool GetIsReloading()  const { return bIsReloading; }
	void SetIsReloading(const bool& IsReloading){ bIsReloading = IsReloading; }
	
	float GetAimOffset_Y() const { return AimOffset_Y; }
	float GetAimOffset_Z() const { return AimOffset_Z; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetSpringArmLength(){ return SpringArmComponent ? SpringArmComponent->TargetArmLength : 0 ; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Weapon)
	FWeaponPickUpInfo GetWeaponPickUpInfo();
	
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

	//角色死亡后做的一些操作
	UFUNCTION()
	void OnRep_Died();

	UFUNCTION()//延迟较高时快速瞄准又退出时，本地会被服务器覆盖了旧时间的状态，将其改为本地最新状态
	void OnRep_IsAiming(){ if(IsLocallyControlled()) bIsAiming = bIsAimingLocally; }

private:
	void HideCharacterIfCameraClose();
public:	
	//当前武器
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, ReplicatedUsing = OnRep_CurrentWeapon)  //Replicated : 网络复制
	class ASWeapon* CurrentWeapon;

	//武器1
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, Replicated)  //Replicated : 网络复制
	class ASWeapon* FirstWeapon;

	//武器2
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon, Replicated)  //Replicated : 网络复制
	class ASWeapon* SecondWeapon;

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
	FOnPickUpWeapon OnPickUpWeapon;

	UPROPERTY(BlueprintAssignable)
	FOnInteractKeyDown OnInteractKeyDown;

	//防止同时与多个可拾取武器发生重叠时间
	bool bHasBeenOverlappedWithPickUpWeapon = false;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USpringArmComponent* SpringArmComponent;
	
	//是否正在开镜变焦
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, ReplicatedUsing = OnRep_IsAiming, Category= Weapon)
	bool bIsAiming;
	//记录客户端本地的瞄准状态，以修正延迟较高时快速瞄准又退出时被服务器覆盖了旧时间的状态
	bool bIsAimingLocally;

	//是否正在射击
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category= Weapon)
	bool bIsFiring;
	
	//当前武器是否正在重新装填子弹
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category= Weapon)
	bool bIsReloading = false;
	
	//默认视野范围
	float DefaultFOV;

	//为玩家生成武器
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<ASWeapon> CurrentWeaponClass;

	//玩家死亡时掉落的可拾取武器类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<class APickUpWeapon> PickUpWeaponClass;

	//武器插槽名称
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	FName WeaponSocketName;

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

	//角色死亡后重生的计时器句柄
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	FTimerHandle FPlayerRespawnTimerHandle;

	//角色重生倒计时
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	float RespawnCount = 5;
	
//临时测试接口的区域
public:
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = TestInterface)
	//bool NotEvent_NativeTest();
	virtual bool NotEvent_NativeTest_Implementation() override;
	
	//virtual void IsEvent_NativeTest_Implementation() override;
	
};
