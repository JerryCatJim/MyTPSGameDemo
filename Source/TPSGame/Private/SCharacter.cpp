// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TPSGame/TPSGame.h"
//#include "Component/SHealthComponent.h"
//#include "Component/SBuffComponent.h"
#include "TPSPlayerController.h"
#include "TPSPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/PickUpWeapon.h"

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
	
	CurrentWeaponSocketName   = "CurrentWeaponSocket";
	MainWeaponSocketName      = "MainWeaponSocket";
	SecondaryWeaponSocketName = "SecondaryWeaponSocket";
	MeleeWeaponSocketName     = "MeleeWeaponSocket";
	ThrowableWeaponSocketName = "ThrowableWeaponSocket";

	bDied = false;
	AimOffset_Y = 0;
	AimOffset_Z = 0;
	bIsAiming = false;
	bIsFiring = false;
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
	
	//如果控制权在服务器Server(相对Client)则执行下列代码
	if(HasAuthority())
	{
		TArray<ASWeapon*> TempWeaponList = {
			MainWeapon,
			SecondaryWeapon,
			MeleeWeapon,
			ThrowableWeapon
		};
		TArray<TEnumAsByte<EWeaponEquipType>> TempEquipTypeList = {
			EWeaponEquipType::MainWeapon,
			EWeaponEquipType::SecondaryWeapon,
			EWeaponEquipType::MeleeWeapon,
			EWeaponEquipType::ThrowableWeapon
		};
		
		//生成默认武器并吸附到角色部位
		for(int i = 0; i < TempWeaponList.Num(); ++i)
		{
			FWeaponPickUpInfo TempInfo = FWeaponPickUpInfo();
			TempInfo.WeaponClass = GetWeaponSpawnClass(TempEquipTypeList[i]);
			
			//武器当前还为空值，取不到自身PickUpWeaponInfo，所以手动造一个，并且不刷新信息(这样造出来的武器的WeaponPickUpInfo的是默认值)
			TempWeaponList[i] = SpawnAndAttachWeapon(TempInfo, false);
			
			if(!CurrentWeapon && TempWeaponList[i])  //把有效的第一把武器设置为当前默认武器
			{
				CurrentWeapon = TempWeaponList[i];
				//C++中的服务器不会自动调用OnRep函数，需要手动调用
				OnRep_CurrentWeapon();
			}
		}
	}
	
	//测试UEC++函数 可以返回多个值
	//testtest();
}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//平滑插值
	const float NewFOV = FMath::FInterpTo(
		CameraComponent->FieldOfView,
		bIsAiming && CurrentWeapon ? CurrentWeapon->ZoomedFOV : DefaultFOV,
		DeltaTime,
		CurrentWeapon ? CurrentWeapon->ZoomInterpSpeed : 20.f);
	
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
	
	if((CameraComponent->GetComponentLocation() - GetActorLocation()).Size() < DistanceToHideCharacter)
	{
		GetMesh()->SetVisibility(false);
		if(IsValid(CurrentWeapon))
		{
			CurrentWeapon->GetWeaponMeshComp()->bOwnerNoSee = true;
			CurrentWeapon->bShowMuzzleFlash = false;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if(IsValid(CurrentWeapon))
		{
			CurrentWeapon->GetWeaponMeshComp()->bOwnerNoSee = false;
			CurrentWeapon->bShowMuzzleFlash = true;
		}
	}
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
	ATPSPlayerController* MyController = GetController<ATPSPlayerController>();
	if(!MyController) return;
	
	ATPSPlayerState* MyPlayerState = MyController->GetPlayerState<ATPSPlayerState>();
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

ASWeapon* ASCharacter::SpawnAndAttachWeapon(FWeaponPickUpInfo WeaponInfo, bool RefreshWeaponInfo)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;  //要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空
	
	ASWeapon* NewWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponInfo.WeaponClass, FTransform(), SpawnParams);
	//NewWeapon->SetOwner(this);
	if(NewWeapon)
	{
		//如果角色身上有对应插槽则吸附，否则说明忘了配置了或者写错名字了，武器就掉到地上
		if(GetMesh()->GetSocketByName(GetWeaponSocketName(NewWeapon->GetWeaponEquipType())))
		{
			NewWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetWeaponSocketName(NewWeapon->GetWeaponEquipType()));
		}
		else
		{
			NewWeapon->SetActorLocation(GetActorLocation());
		}
		if(RefreshWeaponInfo)
		{
			//更新武器信息
			NewWeapon->RefreshWeaponInfo(WeaponInfo);
		}
	}
	return NewWeapon;
}

TSubclassOf<ASWeapon> ASCharacter::GetWeaponSpawnClass(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	TSubclassOf<ASWeapon> WSClass = MainWeaponClass; 
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		WSClass = MainWeaponClass;
		break;
	case EWeaponEquipType::SecondaryWeapon:
		WSClass = SecondaryWeaponClass;
		break;
	case EWeaponEquipType::MeleeWeapon:
		WSClass = MeleeWeaponClass;
		break;
	case EWeaponEquipType::ThrowableWeapon:
		WSClass = ThrowableWeaponClass;
		break;
	default:
		break;
	}
	return WSClass;
}

FName ASCharacter::GetWeaponSocketName(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	FName WSName = MainWeaponSocketName; 
	switch(WeaponEquipType)
	{
		case EWeaponEquipType::MainWeapon:
			WSName = MainWeaponSocketName;
			break;
		case EWeaponEquipType::SecondaryWeapon:
			WSName = SecondaryWeaponSocketName;
			break;
		case EWeaponEquipType::MeleeWeapon:
			WSName = MeleeWeaponSocketName;
			break;
		case EWeaponEquipType::ThrowableWeapon:
			WSName = ThrowableWeaponSocketName;
			break;
		default:
			break;
	}
	return WSName;
}

ASWeapon*& ASCharacter::GetWeaponByEquipType(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	ASWeapon* NoWeapon = nullptr;
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		return MainWeapon;
	case EWeaponEquipType::SecondaryWeapon:
		return SecondaryWeapon;
	case EWeaponEquipType::MeleeWeapon:
		return MeleeWeapon;
	case EWeaponEquipType::ThrowableWeapon:
		return ThrowableWeapon;
	default:
		return NoWeapon;
	}
}

UAnimMontage* ASCharacter::GetSwapWeaponAnim(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		return SwapMainWeaponAnim;
	case EWeaponEquipType::SecondaryWeapon:
		return SwapSecondaryWeaponAnim;
	case EWeaponEquipType::MeleeWeapon:
		return SwapMeleeWeaponAnim;
	case EWeaponEquipType::ThrowableWeapon:
		return SwapThrowableWeaponAnim;
	default:
		return nullptr;
	}
}

float ASCharacter::GetSwapWeaponAnimRate(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	float SwapRate = 0.f;
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		SwapRate = SwapMainWeaponRate;
		break;
	case EWeaponEquipType::SecondaryWeapon:
		SwapRate = SwapSecondaryWeaponRate;
		break;
	case EWeaponEquipType::MeleeWeapon:
		SwapRate = SwapMeleeWeaponRate;
		break;
	case EWeaponEquipType::ThrowableWeapon:
		SwapRate = SwapThrowableWeaponRate;
		break;
	default:
		break;
	}
	return SwapRate;
}

void ASCharacter::DealSwapWeaponAttachment(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	if(!HasAuthority()) return; //只有服务器做实质性逻辑
	
	ASWeapon*& TempWeapon = GetWeaponByEquipType(WeaponEquipType);
	CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetWeaponSocketName(CurrentWeapon->GetWeaponEquipType()));
	TempWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponSocketName);

	CurrentWeapon = GetWeaponByEquipType(WeaponEquipType);
	
	CurrentSwapWeaponAnim = nullptr;

	bIsSwappingWeapon = false;
}

void ASCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);

	//动了才更新AimOffset
	if( !FMath::IsNearlyEqual(Value, 0) )
	{
		SyncAimOffset();
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

	if(CurrentWeapon)
	{
		CurrentWeapon->Destroy();
	}
}

void ASCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if(IsLocallyControlled())
	{
		ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
		if(MyController)  //服务端RestartPlayer时先触发BeginPlay然后OnPossess，BeginPlay时Controller暂时为空，所以在PossessedBy中再触发一次设置准星
		{
			MyController->ResetCrossHair();
		}
	}
}

//SWeapon.cpp中 Fire函数会让Owner(即该文件)调用GetActorEyesViewPoint,下面函数重写了方法，使眼部位置变为摄像机位置
//Character继承于Pawn, Pawn.cpp中 调用GetActorEyesViewPoint 会调用GetPawnViewLocation和GetViewRotation获得值
FVector ASCharacter::GetPawnViewLocation() const
{
	//判断摄像机组件是否为空，然后返回该组件
	if(CameraComponent)
	{
		return CameraComponent->GetComponentLocation() + CameraComponent->GetForwardVector();
	}
	return Super::GetPawnViewLocation();
}

//将开镜行为发送到服务器然后同步
void ASCharacter::SetWeaponZoom()
{
	if(bDisableGamePlayInput) return;

	//空手不能开镜
	if(CurrentWeapon)
	{
		CurrentWeapon->SetWeaponZoom();
		if(IsLocallyControlled())
		{
			bIsAiming = true;
			bIsAimingLocally = true;
		}
	}
}

void ASCharacter::ResetWeaponZoom()
{
	if(bDisableGamePlayInput) return;

	if(CurrentWeapon)
	{
		CurrentWeapon->ResetWeaponZoom();
		if(IsLocallyControlled())
		{
			bIsAiming = false;
			bIsAimingLocally = false;
		}
	}
}

void ASCharacter::PickUpWeapon(FWeaponPickUpInfo WeaponInfo)
{
	//要先StopFire再StopReload，否则会导致0子弹时更换武器，StopFire中因为0子弹又执行了一次Reload，生成武器后的新武器子弹数没及时刷新时又执行一次换弹
	//(当然，你也可以生成时再执行一次StopReload，但我感觉那样不好)
	StopFire();
	StopReload();
	
	//客户端或服务端都应在拾取武器时停止开火，但是生成新武器等操作仅在服务端执行
	DealPickUpWeapon(WeaponInfo);
}

void ASCharacter::DealPickUpWeapon_Implementation(FWeaponPickUpInfo WeaponInfo)
{
	const FWeaponPickUpInfo LastWeaponInfo = GetWeaponPickUpInfo();
	
	//把旧武器信息广播出去，可用于和地上可拾取武器的信息互换
	OnPickUpWeapon.Broadcast(LastWeaponInfo);

	ASWeapon*& WeaponToExchange = GetWeaponByEquipType(WeaponInfo.WeaponEquipType);
	/*if(!WeaponToExchange)  //如果没装备对应位置的武器则直接拾取
	{
		WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
		if(!CurrentWeapon || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::MeleeWeapon || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::ThrowableWeapon)
		{
			StartSwapWeapon(WeaponToExchange->GetWeaponEquipType(), true);
		}
	}
	else
	{
		WeaponToExchange->Destroy(true);
		WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
		if(!CurrentWeapon)  //如果当前武器类型和要拾取的类型相同，则销毁后CurrentWeapon会为空，此时执行交换逻辑正好
		{
			StartSwapWeapon(WeaponToExchange->GetWeaponEquipType(), true);
		}
	}*/
	//以上两段代码可继续简化为如下
	WeaponToExchange->Destroy(true);
	WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
	if(!CurrentWeapon || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::MeleeWeapon || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::ThrowableWeapon)
	{
		StartSwapWeapon(WeaponToExchange->GetWeaponEquipType(), true);
	}
}

void ASCharacter::DropWeapon_Implementation()
{
	if(!HasAuthority())  //客户端不要生成武器，等待服务端生成后复制
	{
		return;
	}
	if(!CurrentWeapon || !CurrentWeapon->GetWeaponCanDropDown())
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if(World)
	{
		APickUpWeapon* PickUpWeapon = World->SpawnActorDeferred<APickUpWeapon>(
			PickUpWeaponClass,
			FTransform(GetActorLocation())
			);
		if(PickUpWeapon)
		{
			PickUpWeapon->WeaponPickUpInfo = FWeaponPickUpInfo(
				this,
				CurrentWeapon->GetWeaponMeshComp()->SkeletalMesh,
				GetWeaponSpawnClass(CurrentWeapon->GetWeaponEquipType()),
				CurrentWeapon->CurrentAmmoNum,
				CurrentWeapon->BackUpAmmoNum,
				CurrentWeapon->WeaponName,
				CurrentWeapon->GetWeaponEquipType()
				);
			//从C++中获取蓝图类
			const FString WidgetClassLoadPath = FString(TEXT("/Game/UI/WBP_ItemPickUpTip.WBP_ItemPickUpTip_C"));//蓝图一定要加_C这个后缀名
			UClass* Widget = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
			PickUpWeapon->WidgetComponent->SetWidgetClass(Widget);
			PickUpWeapon->bCanMeshDropOnTheGround = true;

			//主动扔掉的武器不能长按刷新为原武器(场景内摆放的或者什么道具点刷新的才可以)
			PickUpWeapon->bCanInteractKeyLongPress = false;
			
			PickUpWeapon->FinishSpawning(FTransform(GetActorLocation()));
		}
	}
}

void ASCharacter::StartFire()
{
	if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		//有子弹或者无限子弹才设置为开火状态，防止一按开火人物就瞬间面对正前方
		CurrentWeapon->StartFire();
		if(IsLocallyControlled())
		{
			bIsFiring = true;
			bIsFiringLocally = true;
		}
	}
}

void ASCharacter::StopFire()
{
	//if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		CurrentWeapon->StopFire();
		if(IsLocallyControlled())
		{
			bIsFiring = false;
			bIsFiringLocally = false;
		}
	}
}

void ASCharacter::StartReload()
{
	if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		CurrentWeapon->StartReload();
		if(IsLocallyControlled())
		{
			bIsReloading = true;
			bIsReloadingLocally = true;
		}
	}
}

void ASCharacter::StopReload()
{
	//if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		CurrentWeapon->StopReload();
		if(IsLocallyControlled())
		{
			bIsReloading = false;
			bIsReloadingLocally = false;
		}
	}
}

void ASCharacter::StartSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(bDisableGamePlayInput) return;
	if(GetWeaponByEquipType(NewWeaponEquipType))
	{
		SwapWeapon(NewWeaponEquipType, Immediately);
		if(IsLocallyControlled())
		{
			bIsSwappingWeapon = !Immediately;
			bIsSwappingWeaponLocally = !Immediately;
		}
	}
}

void ASCharacter::StopSwapWeapon()
{
	GetWorldTimerManager().ClearTimer(SwapWeaponTimer);
	StopAnimMontage(CurrentSwapWeaponAnim);
	
	if(IsLocallyControlled())
	{
		bIsSwappingWeapon = false;
		bIsSwappingWeaponLocally = false;
	}
}

void ASCharacter::SwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	StopFire();
	StopReload();
	
	if(!HasAuthority())  //本地只处理一些表现效果，权威端在服务器
	{
		//分开处理是为了高延迟下客户端也能立刻响应一些表现
		ServerSwapWeapon(NewWeaponEquipType, Immediately);
		LocalSwapWeapon(NewWeaponEquipType, Immediately);
		return;
	}
	
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
}

void ASCharacter::LocalSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
}

void ASCharacter::ServerSwapWeapon_Implementation(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	SwapWeapon(NewWeaponEquipType, Immediately);
}

void ASCharacter::DealPlaySwapWeaponAnim(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(Immediately)
	{
		if(HasAuthority())  //客户端只播放动画
		{
			DealSwapWeaponAttachment(NewWeaponEquipType);
		}
	}
	else
	{
		UAnimMontage* SwapAnim = GetSwapWeaponAnim(NewWeaponEquipType);
		float SwapAnimPlayRate = GetSwapWeaponAnimRate(NewWeaponEquipType);
		float MontagePlayTime = SwapAnim && SwapAnimPlayRate>0.f ? SwapAnim->SequenceLength/SwapAnimPlayRate : 0.f ;
		
		//如果上次切换动画播到一半又想切回原武器，只需反播当前动画
		if(bIsSwappingWeapon && NewWeaponEquipType == CurrentWeapon->GetWeaponEquipType())
		{
			SwapAnim = CurrentSwapWeaponAnim;
			
			const UAnimInstance* AnimIns = GetMesh()->GetAnimInstance();
			MontagePlayTime = (SwapAnim->SequenceLength - AnimIns->Montage_GetPosition(SwapAnim))/SwapAnimPlayRate;
			
			//播放速率变为复数即为反着播放(?)
			SwapAnimPlayRate = SwapAnimPlayRate * -1.f;
		}
		if(SwapAnim)
		{
			if(MontagePlayTime > 0)
			{
				CurrentSwapWeaponAnim = SwapAnim;
				
				PlayAnimMontage(SwapAnim, SwapAnimPlayRate);
				
				GetWorldTimerManager().SetTimer(
					SwapWeaponTimer,
					[this,&]()
					{
						StopAnimMontage(CurrentSwapWeaponAnim);
						if(SwapAnimPlayRate > 0)
						{
							if(HasAuthority())  //客户端只播放动画
							{
								DealSwapWeaponAttachment(NewWeaponEquipType);
							}
						}
						//如果SwapAnimPlayRate < 0说明在反向播放，之前的武器还没切换完成，不需要实际地切换Attachment
					},
					MontagePlayTime*0.8,
					false);
			}
			else
			{
				const UEnum* TestEnumPtr = StaticEnum<EWeaponEquipType>();
				GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red,
					FString::Printf(TEXT("Swap%sAnim蒙太奇不存在！"),*TestEnumPtr->GetDisplayNameTextByValue(static_cast<uint8>(NewWeaponEquipType)).ToString()));
				
				if(HasAuthority())  //客户端只播放动画
				{
					DealSwapWeaponAttachment(NewWeaponEquipType);
				}
			}
		}
	}
}


void ASCharacter::SetPlayerControllerRotation_Implementation()
{
	bUseControllerRotationYaw = !bDied && (bIsAiming || bIsFiring);
	GetCharacterMovement()->bOrientRotationToMovement = !bDied && !(bIsAiming || bIsFiring);
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

void ASCharacter::OnRep_CurrentWeapon()
{
	ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
	if(MyController)  //服务端RestartPlayer时先触发BeginPlay然后OnPossess，BeginPlay时Controller暂时为空，所以在PossessedBy中再触发一次设置准星
	{
		MyController->ResetCrossHair();
	}
	
	OnCurrentWeaponChanged.Broadcast();
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
	StopSwapWeapon();

	if(HasAuthority())
	{
		DropWeapon();  //死亡后掉落武器,然后隐藏手中武器

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
	
	if(CurrentWeapon && CurrentWeapon->GetWeaponMeshComp())
	{
		CurrentWeapon->GetWeaponMeshComp()->SetVisibility(false);
		CurrentWeapon->ResetWeaponZoom();
	}
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

	PlayerTeam = Team;
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

FWeaponPickUpInfo ASCharacter::GetWeaponPickUpInfo()
{
	if(CurrentWeapon)
	{
		return CurrentWeapon->GetWeaponPickUpInfo();
	}
	return FWeaponPickUpInfo();
}

//一个模板
void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	
	DOREPLIFETIME(ASCharacter, MainWeapon);
	DOREPLIFETIME(ASCharacter, SecondaryWeapon);
	DOREPLIFETIME(ASCharacter, MeleeWeapon);
	DOREPLIFETIME(ASCharacter, ThrowableWeapon);
	
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, bIsAiming);
	DOREPLIFETIME(ASCharacter, bIsFiring);
	DOREPLIFETIME(ASCharacter, bIsReloading);
	DOREPLIFETIME(ASCharacter, bIsSwappingWeapon);
	
	DOREPLIFETIME(ASCharacter, AimOffset_Y);
	DOREPLIFETIME(ASCharacter, AimOffset_Z);
	DOREPLIFETIME(ASCharacter, bDisableGamePlayInput);
	DOREPLIFETIME(ASCharacter, PlayerTeam);
}

bool ASCharacter::NotEvent_NativeTest_Implementation()
{
	return true;
}
