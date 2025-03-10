// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TPSPlayerState.h"
#include "Weapon/BaseWeapon/SWeapon.h"
#include "GameFramework/Character.h"
#include "Interface/MyInterfaceTest.h"
#include "TPSGameType/Team.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Component/WeaponManagerComponent.h"
#include "SCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyDown);  //对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件，用以区分
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractKeyLongPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPlayerDead, AController*, InstigatedBy, AActor*, DamageCauser,const UDamageType*, DamageType);

class UInputComponent;
class UCameraComponent;
class USpringArmComponent;
class USHealthComponent;
class USBuffComponent;
class UWeaponManagerComponent;

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
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//角色被销毁时调用
	virtual void Destroyed() override;

	//通常只在服务端触发, 刚进游戏时服务端先OnPossess然后BeginPlay，RestartPlayer后服务端先BeginPlay然后OnPossess
	virtual void PossessedBy(AController* NewController) override;

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

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FName GetCharacterName();
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsAiming() const { return WeaponManagerComponent->GetIsAiming(); }
	UFUNCTION(BlueprintCallable)
	void SetIsAiming(const bool& IsAiming){ WeaponManagerComponent->SetIsAiming(IsAiming); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsFiring() const { return WeaponManagerComponent->GetIsFiring(); }
	UFUNCTION(BlueprintCallable)
	void SetIsFiring(const bool& IsFiring){ WeaponManagerComponent->SetIsFiring(IsFiring); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsReloading()  const { return WeaponManagerComponent->GetIsReloading(); }
	UFUNCTION(BlueprintCallable)
	void SetIsReloading(const bool& IsReloading){ WeaponManagerComponent->SetIsReloading(IsReloading); }
	void SetIsReloadingLocally(const bool& IsReloading){ WeaponManagerComponent->SetIsReloadingLocally(IsReloading); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsSwappingWeapon()  const { return WeaponManagerComponent->GetIsSwappingWeapon(); }
	UFUNCTION(BlueprintCallable)
	void SetIsSwappingWeapon(const bool& IsSwappingWeapon){ WeaponManagerComponent->SetIsSwappingWeapon(IsSwappingWeapon); }
	
	//重写,获取摄像机组件位置
	virtual FVector GetPawnViewLocation() const override;

	UCameraComponent* GetCameraComponent()  const { return CameraComponent; }
	
	USHealthComponent* GetHealthComponent() const { return HealthComponent; }
	USBuffComponent* GetBuffComponent()     const { return BuffComponent; }
	UWeaponManagerComponent* GetWeaponManagerComponent() const { return WeaponManagerComponent; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UInventoryComponent* GetInventoryComponent();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category=CurrentWeapon)
	ASWeapon* GetCurrentWeapon(){ return WeaponManagerComponent->CurrentWeapon; }
	
	bool GetIsDied() const { return bDied; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category=PlayerTeam)
	ETeam GetTeam() const
	{
		ATPSPlayerState* PS = Cast<ATPSPlayerState>(GetPlayerState());
		if(PS)
		{
			return PS->GetTeam();
		}
		return ETeam::ET_NoTeam;
	}
	UFUNCTION(BlueprintCallable, Category=PlayerTeam)
	void SetTeam(ETeam TeamID)
	{
		ATPSPlayerState* PS = Cast<ATPSPlayerState>(GetPlayerState());
		if(PS)
		{
			PS->SetTeam(TeamID);
		}
	}
	
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
	
	//根据是否开镜设置人物的移动类型
	UFUNCTION(BlueprintNativeEvent)
	void SetPlayerControllerRotation();

	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void SyncAimOffset();  //放在了LookUp()中调用
	
	//生命值更改函数
	UFUNCTION()//Server, Reliable)  //必须UFUNCTION才能绑定委托
	void OnHealthChanged(USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, //HealthDelta 生命值改变量,增加或减少
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	//角色死亡后做的一些操作
	UFUNCTION()
	void OnRep_Died();
	
private:
	void HideCharacterIfCameraClose();
	void TryInitBodyColor();
	void LoopSetBodyColor();

	UFUNCTION()
	void OnRep_AimOffset_Y(){ if(IsLocallyControlled()) AimOffset_Y = AimOffset_Y_Locally; }
	UFUNCTION()
	void OnRep_AimOffset_Z(){ if(IsLocallyControlled()) AimOffset_Z = AimOffset_Z_Locally; }
	
public:	
	UPROPERTY(EditAnywhere)
	float DistanceToHideCharacter = 100;

	//禁止除了旋转镜头以外的游戏操作(例如移动，开火等)输入
	UPROPERTY(Replicated)
	bool bDisableGamePlayInput = false;

	UPROPERTY(BlueprintAssignable)
	FPlayerDead OnPlayerDead;
	
	UPROPERTY(BlueprintAssignable)  //对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件
	FOnInteractKeyUp OnInteractKeyDown;
	UPROPERTY(BlueprintAssignable)
	FOnInteractKeyUp OnInteractKeyUp;
	UPROPERTY(BlueprintAssignable)
	FOnInteractKeyLongPressed OnInteractKeyLongPressed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bIsAIPlayer = false;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= CharacterName)
	FName CharacterName =TEXT("默认名字");
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USpringArmComponent* SpringArmComponent;
	
	//默认视野范围
	float DefaultFOV;

	//生命值组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USHealthComponent* HealthComponent;

	//Buff组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	USBuffComponent* BuffComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= Component)
	UWeaponManagerComponent* WeaponManagerComponent;
	
	//角色是否死亡
	UPROPERTY(ReplicatedUsing = OnRep_Died, BlueprintReadOnly, Category= PlayerStatus)
	bool bDied;

	//瞄准偏移量
	UPROPERTY(ReplicatedUsing = OnRep_AimOffset_Y, BlueprintReadWrite, Category= PlayerStatus)
	float AimOffset_Y;
	UPROPERTY(ReplicatedUsing = OnRep_AimOffset_Z, BlueprintReadWrite, Category= PlayerStatus)
	float AimOffset_Z;

	//角色重生倒计时
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayerTimer)
	float RespawnCount = 5;
	
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
	
private:
	bool bPlayerLeftGame = false;
	
	FTimerHandle FGetPlayerStateHandle;
	int TryGetPlayerStateTimes = 0;  //超过5次还没成功就停止计时器

	float AimOffset_Y_Locally;
	float AimOffset_Z_Locally;
	
//临时测试接口的区域
public:
	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = TestInterface)
	//bool NotEvent_NativeTest();
	virtual bool NotEvent_NativeTest_Implementation() override;
	
	//virtual void IsEvent_NativeTest_Implementation() override;
	
};
