// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TPSPlayerController.h"
#include "TPSPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Component/SBuffComponent.h"
#include "Component/SHealthComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/SkillComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/PickUpWeapon.h"
#include "TPSGameType/CustomCollisionType.h"

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//设置蹲起生效
    //ACharacter::GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	//打开组件的网络同步，这两句好像没用
	GetMovementComponent()->SetIsReplicated(true);
	//GetCapsuleComponent()->SetIsReplicated(true);

	//生成蓝图后再在构造函数里修改碰撞设置，蓝图中设置不会改变，所以挪到BeginPlay中再触发一次
	//胶囊体对Weapon通道忽略,防止阻挡射线对Mesh的检测
	//GetCapsuleComponent()->SetCollisionResponseToChannel(Collision_Weapon, ECR_Ignore);
	
	//创建弹簧臂组件
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->TargetArmLength = 150;
	
	//创建摄像机组件
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->SetRelativeLocation(FVector(0,50,60));

	//调整聚焦景深，让摄像机放大画面时看远处更清晰
	CameraComponent->PostProcessSettings.DepthOfFieldFocalDistance = 10000.f;  //DepthOfFieldFocalDistance就是蓝图中CameraComponent的PostProcess/Lens/DepthOfField/FocalDistance
	//调整光圈(?)，让摄像机放大画面时看近处更清晰
	CameraComponent->PostProcessSettings.DepthOfFieldFstop = 32.f;  //DepthOfFieldFstop就是蓝图中CameraComponent的PostProcess/Lens/Camera/Aperture(F-Stop)
	
	//设置Mesh的Transform
	GetMesh()->SetRelativeLocation(FVector(0,0,-87));
	GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
	
	//生命值组件初始化
	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));
	//Buff组件初始化
	BuffComponent = CreateDefaultSubobject<USBuffComponent>(TEXT("BuffComponent"));
	WeaponManagerComponent = CreateDefaultSubobject<UWeaponManagerComponent>(TEXT("WeaponManagerComponent"));
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	
	bDied = false;
	AimOffset_Y = 0;
	AimOffset_Z = 0;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//胶囊体对Weapon通道忽略,防止阻挡射线对Mesh的检测
	GetCapsuleComponent()->SetCollisionResponseToChannel(Collision_Weapon, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(Collision_Projectile, ECR_Ignore);
	
	//储存为初始值
	DefaultFOV = CameraComponent->FieldOfView;
	
	if(HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);
	}
	
	//初始化身体颜色
	TryInitBodyColor();
	
	//测试UEC++函数 可以返回多个值
	//testtest();
}

void ASCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	/*//PlayerController.cpp的OnPossess函数中先触发Pawn的PossessedBy再触发SetPawn()，所以走到这里时，controller还没有新的Pawn，
	//HUD里判断GetPawn()就会失败，所以这段代码也就没用了。把逻辑移到Controller的OnPossess中实现
	if(IsLocallyControlled())
	{
		ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
		if(MyController)  //服务端RestartPlayer时先触发BeginPlay然后OnPossess，BeginPlay时Controller暂时为空，所以在PossessedBy中再触发一次设置UI
		{
			MyController->ResetHUDWidgets(EHUDViewType::AllViews);
		}
	}*/
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//平滑插值
	const float NewFOV = FMath::FInterpTo(
		CameraComponent->FieldOfView,
		GetIsAiming() && WeaponManagerComponent->CurrentWeapon ? WeaponManagerComponent->CurrentWeapon->ZoomedFOV : DefaultFOV,
		DeltaTime,
		WeaponManagerComponent->CurrentWeapon ? WeaponManagerComponent->CurrentWeapon->ZoomInterpSpeed : 20.f);
	//设置开镜变焦效果
	CameraComponent->SetFieldOfView(NewFOV);

	//根据是否开镜设置人物的移动类型
	SetPlayerControllerRotation();

	HideCharacterIfCameraClose();
}

void ASCharacter::OnMatchEnd(int NewWinnerID, ETeam NewWinningTeam)
{
	StopFire();
	GetCharacterMovement()->StopMovementImmediately();
}

void ASCharacter::HideCharacterIfCameraClose()
{
	//只在本地运行
	if(!IsLocallyControlled())
	{
		return;
	}

	const bool bVisible = (CameraComponent->GetComponentLocation() - GetActorLocation()).Size() >= DistanceToHideCharacter;
	GetMesh()->SetVisibility(bVisible);
	WeaponManagerComponent->SetCurrentWeaponVisibility(bVisible);
}

void ASCharacter::TryInitBodyColor()
{
	UWorld* World = GetWorld();
	if(World)
	{
		GetWorldTimerManager().SetTimer(
			FGetPlayerStateHandle,
			this,
			&ASCharacter::LoopSetBodyColor,  //用匿名函数会闪退(?)
			0.2,
			true,
			0
			);
	}
}

void ASCharacter::LoopSetBodyColor()
{
	ATPSPlayerState* MyPlayerState = GetPlayerState<ATPSPlayerState>();
	if(MyPlayerState)
	{
		SetBodyColor(MyPlayerState->GetTeam());
		GetWorldTimerManager().ClearTimer(FGetPlayerStateHandle);
	}
	else
	{
		TryGetPlayerStateTimes++;
		if(TryGetPlayerStateTimes >= 5 )
		{
			//超过5次还没成功就停止计时器
			GetWorldTimerManager().ClearTimer(FGetPlayerStateHandle);
		}
	}
}

void ASCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);

	//动了才更新AimOffset
	if( !FMath::IsNearlyEqual(Value, 0) )
	{
		SyncAimOffset();
		//高延迟下本地直接先行修改以保持流畅
		if(IsLocallyControlled())
		{
			FRotator TargetRotator = GetControlRotation()-GetActorRotation();
			TargetRotator.Normalize();
	
			AimOffset_Y = FMath::RInterpTo(
				FRotator(AimOffset_Y,AimOffset_Z, 0),
				TargetRotator,
				GetWorld()->GetDeltaSeconds(),
				5
				).Pitch;
	
			AimOffset_Y = TargetRotator.Pitch;
			AimOffset_Z = 0;
			AimOffset_Y_Locally = AimOffset_Y;
			AimOffset_Z_Locally = AimOffset_Z;
		}
	}
}

void ASCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASCharacter::MoveForward(float Value)
{
	if(bDisableGamePlayInput) return;
	FVector ForwardV = FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetScaledAxis(EAxis::X);
	AddMovementInput(ForwardV, Value);
	//AddMovementInput(FRotator(0,GetControlRotation().Yaw,0).Vector(), Value);
	//AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacter::MoveRight(float Value)
{
	if(bDisableGamePlayInput) return;
	FVector RightV = FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetScaledAxis(EAxis::Y);
	AddMovementInput(RightV, Value);
	//AddMovementInput(GetActorRightVector() * Value);  //bUseControllerRotationYaw为false且GetCharacterMovement()->bOrientRotationToMovement时，会导致原地打转
}

void ASCharacter::BeginJump()
{
	if(bDisableGamePlayInput) return;
	Jump();
}

void ASCharacter::EndJump()
{
	if(bDisableGamePlayInput) return;
	StopJumping();
}

void ASCharacter::BeginCrouch()
{
	if(bDisableGamePlayInput) return;
	Crouch();
}

void ASCharacter::EndCrouch()
{
	if(bDisableGamePlayInput) return;
	UnCrouch();
}

void ASCharacter::InteractKeyPressed()//_Implementation()
{
	if(bDisableGamePlayInput || bDied) return;

	//对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件，用以区分
	//由于交互按钮会对不同物体产生不同反馈，应将这一行为广播出去，由要产生互动的一端绑定委托，然后收到广播后自行编写响应行为
	OnInteractKeyDown.Broadcast();
	
	TryLongPressInteractKey();
}

void ASCharacter::InteractKeyReleased()//_Implementation()
{
	if(bDisableGamePlayInput || bDied) return;

	//松开按键就清除长按计时器
	GetWorldTimerManager().ClearTimer(FInteractKeyLongPressBeginHandle);
	GetWorldTimerManager().ClearTimer(FInteractKeyLongPressFinishHandle);
	
	if(!bIsLongPressing) //如果按住时间太短，未进入长按状态，则执行普通按键逻辑
	{
		OnInteractKeyUp.Broadcast();
	}
	bIsLongPressing = false;
}

void ASCharacter::TryLongPressInteractKey()//_Implementation()
{
	if(bDisableGamePlayInput || bDied) return;

	//进入长按状态
	GetWorldTimerManager().SetTimer(
	FInteractKeyLongPressBeginHandle,
	this,
	&ASCharacter::BeginLongPressInteractKey,
	InteractKeyLongPressBeginSecond,
	false);
}

void ASCharacter::BeginLongPressInteractKey()
{
	bIsLongPressing = true;
	//例如 长按0.5秒后进入长安状态后，在 2 - 0.5 秒后执行长按完成的广播
	GetWorldTimerManager().SetTimer(
	FInteractKeyLongPressFinishHandle,
	[this]()->void
	{
		//由于交互按钮会对不同物体产生不同反馈，应将这一行为广播出去，由要产生互动的一端绑定委托，然后收到广播后自行编写响应行为
		OnInteractKeyLongPressed.Broadcast();
	},
	InteractKeyLongPressFinishSecond - InteractKeyLongPressBeginSecond,
	false);
}

void ASCharacter::PickUpWeapon(FWeaponPickUpInfo WeaponInfo)
{
	WeaponManagerComponent->PickUpWeapon(WeaponInfo);
}

void ASCharacter::StartDropWeapon(bool ManuallyDiscard)
{
	WeaponManagerComponent->StartDropWeapon(ManuallyDiscard);
}

void ASCharacter::StartFire()
{
	if(bDisableGamePlayInput) return;
	
	WeaponManagerComponent->StartFire();
}

void ASCharacter::StopFire()
{
	//if(bDisableGamePlayInput) return;
	WeaponManagerComponent->StopFire();
}

void ASCharacter::StartReload()
{
	if(bDisableGamePlayInput) return;
	
	WeaponManagerComponent->StartReload();
}

void ASCharacter::StopReload()
{
	//if(bDisableGamePlayInput) return;
	WeaponManagerComponent->StopReload();
}

void ASCharacter::StartSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(bDisableGamePlayInput) return;
	
	WeaponManagerComponent->StartSwapWeapon(NewWeaponEquipType, Immediately);
}

void ASCharacter::StopSwapWeapon(bool bWaitCurrentWeaponReplicated)
{
	WeaponManagerComponent->StopSwapWeapon(bWaitCurrentWeaponReplicated);
}

void ASCharacter::SetWeaponZoom()
{
	if(bDisableGamePlayInput) return;
	
	WeaponManagerComponent->SetWeaponZoom();
}

void ASCharacter::ResetWeaponZoom()
{
	if(bDisableGamePlayInput) return;
	
	WeaponManagerComponent->ResetWeaponZoom();
}

FName ASCharacter::GetCharacterName()
{
	return CharacterName;
}

//由于被交互的物体可能为ROLE_SimulatedProxy,从客户端发送的Server版RPC调用会被丢弃，Multicast也仅会本地调用，
//所以有时需从那边手动触发SCharacter的Server版事件广播以完成 被交互物体 的Server端逻辑响应
void ASCharacter::Server_OnInteractKeyDown_Implementation()
{
	OnInteractKeyDown.Broadcast();
}
void ASCharacter::Server_OnInteractKeyUp_Implementation()
{
	OnInteractKeyUp.Broadcast();
}
void ASCharacter::Server_OnInteractKeyLongPressed_Implementation()
{
	OnInteractKeyLongPressed.Broadcast();
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//绑定动作映射
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::BeginJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASCharacter::EndJump);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);
	
	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::SetWeaponZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::ResetWeaponZoom);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASCharacter::InteractKeyPressed);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ASCharacter::InteractKeyReleased);
	
	//绑定轴映射输入
	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::Turn);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);
}

void ASCharacter::Destroyed()
{
	Super::Destroyed();
}

//SWeapon.cpp中 Fire函数会让Owner(即该文件)调用GetActorEyesViewPoint,下面函数重写了方法，使眼部位置变为摄像机位置
//Character继承于Pawn, Pawn.cpp中 调用GetActorEyesViewPoint 会调用GetPawnViewLocation和GetViewRotation获得值
FVector ASCharacter::GetPawnViewLocation() const
{
	//AIController里的UpdateControlRotation会调用此函数计算Controller的旋转，而且BP_AI继承于SCharacter，所以不希望AI调用
	if(!bIsAIPlayer && CameraComponent)
	{
		return CameraComponent->GetComponentLocation() + CameraComponent->GetForwardVector();
	}
	return Super::GetPawnViewLocation();
}

void ASCharacter::SetPlayerControllerRotation_Implementation()
{
	bUseControllerRotationYaw = !bDied && (GetIsAiming() || GetIsFiring());
	GetCharacterMovement()->bOrientRotationToMovement = !bDied && !(GetIsAiming() || GetIsFiring());
}

void ASCharacter::OnHealthChanged(class USHealthComponent* OwningHealthComponent, float Health, float HealthDelta, //HealthDelta 生命值改变量,增加或减少
                                  const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if(Health <= 0.f && !bDied)
	{
		if(HasAuthority())
		{
			bDied = true;
			OnRep_Died();
			//BP_PlayerCharacter蓝图里绑定了, PlayerDead_Bind函数
			OnPlayerDead.Broadcast(InstigatedBy, DamageCauser, DamageType);
		}
	}
}

void ASCharacter::OnRep_Died()
{
	if(!bDied) return;
	
	//角色死亡
	GetMovementComponent()->UNavMovementComponent::StopMovementImmediately();  //无法移动  4.27以上用GetMovementComponent()->StopMovement会出错(？)
	GetCharacterMovement()->DisableMovement();
	bDisableGamePlayInput = true;
	
	StopFire();
	StopReload();
	StopSwapWeapon(false);

	if(HasAuthority())
	{
		StartDropWeapon(false);  //死亡后掉落武器,然后隐藏手中武器

		if(!bPlayerLeftGame)
		{
			//尝试复活
			ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
			if(MyController)
			{
				MyController->RequestRespawn();
			}
		}
	}
	//销毁所有武器
	WeaponManagerComponent->DestroyAllWeapons();
	
	ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
	if(MyController)
	{
		MyController->RemoveCrossHair();
	}
		
	//获取胶囊体碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//死亡后Mesh仍处于站立状态？所以设置为无碰撞响应防止Projectile子弹继续击中虚空Mesh
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASCharacter::PlayerLeaveGame_Implementation()
{
	bPlayerLeftGame = true;
	if(!bDied)
	{
		bDied = true;
		OnRep_Died();
	}
}

void ASCharacter::SetBodyColor(ETeam Team)
{
	if( GetMesh() == nullptr || (Team == ETeam::ET_NoTeam && OriginalMaterial == nullptr) ) return;
	
	switch(Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		break;
	default:
		break;
	}
}

UInventoryComponent* ASCharacter::GetInventoryComponent()
{
	ATPSPlayerController* PC = GetController<ATPSPlayerController>();
	if(PC)
	{
		UE_LOG(LogTemp, Error, TEXT("PC存在!"));
		return PC->InventoryComponent;
	}
	UE_LOG(LogTemp, Error, TEXT("PC无效!"));
	return nullptr;
}

void ASCharacter::SyncAimOffset_Implementation()
{
	FRotator TargetRotator = GetControlRotation()-GetActorRotation();
	TargetRotator.Normalize();
	
	AimOffset_Y = FMath::RInterpTo(
		FRotator(AimOffset_Y,AimOffset_Z, 0),
		TargetRotator,
		GetWorld()->GetDeltaSeconds(),
		5
		).Pitch;
	
	AimOffset_Y = TargetRotator.Pitch;
	AimOffset_Z = 0;
}

//一个模板
void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, AimOffset_Y);
	DOREPLIFETIME(ASCharacter, AimOffset_Z);
	DOREPLIFETIME(ASCharacter, bDisableGamePlayInput);
	DOREPLIFETIME(ASCharacter, bIsAIPlayer);
}

bool ASCharacter::NotEvent_NativeTest_Implementation()
{
	return true;
}
