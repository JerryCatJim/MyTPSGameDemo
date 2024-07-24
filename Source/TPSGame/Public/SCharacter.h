// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SWeapon.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Component/SBuffComponent.h"
#include "Component/SHealthComponent.h"
#include "Component/InventoryComponent.h"
#include "GameFramework/SpringArmComponent.h"
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

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Weapon)
	void PickUpWeapon(FWeaponPickUpInfo WeaponInfo);
	
	//控制武器开火
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StartFire();
	//停止射击
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StopFire();
	//停止装填子弹
	UFUNCTION(BlueprintCallable, Category= WeaponFire)
	void StopReload();

	//设置是否开镜
	UFUNCTION(BlueprintCallable, Server,Reliable)  //将开镜行为发送到服务器然后同步
	void SetZoomFOV();
	UFUNCTION(BlueprintCallable, Server,Reliable)  //将开镜行为发送到服务器然后同步
	void ResetZoomFOV();

	//当交互键(E)被按下时
	UFUNCTION(BlueprintCallable)//, Server, Reliable)
	void InteractKeyPressed();
	UFUNCTION(BlueprintCallable)//, Server, Reliable)
	void InteractKeyReleased();
	
	UFUNCTION(Server,Reliable)  //将射击行为发送到服务器然后同步
	void SetIsFiring(bool IsFiring);
	
	//重写,获取摄像机组件位置
	virtual FVector GetPawnViewLocation() const override;

	USHealthComponent* GetHealthComponent();
	USBuffComponent* GetBuffComponent();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UInventoryComponent* GetInventoryComponent();

	bool GetIsDied();

	bool GetIsAiming();

	bool GetIsFiring();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsReloading();
	
	float GetAimOffset_Y();
	float GetAimOffset_Z();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetSpringArmLength();
	
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
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(BlueprintAssignable)
	FCurrentWeaponChanged OnCurrentWeaponChanged;

	UPROPERTY(BlueprintAssignable)
	FPlayerDead OnPlayerDead;

	UPROPERTY(BlueprintAssignable)
	FOnPickUpWeapon OnPickUpWeapon;

	UPROPERTY(BlueprintAssignable)
	FOnInteractKeyDown OnInteractKeyDown;
	
	//是否正在开镜变焦
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category= Weapon)
	bool bIsAiming;

	//是否正在射击
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category= Weapon)
	bool bIsFiring;

	//是否正在持有武器(对应空手和拿着武器)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category= Weapon)
	bool bIsUsingWeapon;
	
	//开镜后的视野范围
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponZoom)
	float ZoomedFOV;

	//默认视野范围
	float DefaultFOV;

	//开镜速度
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= WeaponZoom, meta=(ClampMin = 0.1f, ClampMax = 100.f))
	float ZoomInterpSpeed;

	//为玩家生成武器
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= Weapon)
	TSubclassOf<ASWeapon> CurrentWeaponClass;

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
	
//临时测试接口的区域
public:
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = TestInterface)
	//bool NotEvent_NativeTest();
	virtual bool NotEvent_NativeTest_Implementation() override;
	
	//virtual void IsEvent_NativeTest_Implementation() override;
	
};
