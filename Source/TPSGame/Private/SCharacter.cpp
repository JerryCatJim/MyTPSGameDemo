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
		//生成默认武器并吸附到角色部位
		for(int i = 0; i < WeaponEquipTypeList.Num(); ++i)
		{
			FWeaponPickUpInfo TempInfo = FWeaponPickUpInfo();
			TempInfo.WeaponClass = GetWeaponSpawnClass(WeaponEquipTypeList[i]);
			
			//武器当前还为空值，取不到自身PickUpWeaponInfo，所以手动造一个，并且不刷新信息(这样造出来的武器的WeaponPickUpInfo的是默认值)
			ASWeapon*& CurWeapon = GetWeaponByEquipType(WeaponEquipTypeList[i]) = SpawnAndAttachWeapon(TempInfo, false);
			
			if(!CurrentWeapon && CurWeapon)  //把有效的第一把武器设置为当前默认武器
			{
				CurrentWeapon = CurWeapon;
				CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponSocketName);
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

	//如果Spawn的Class不存在则不会生成
	ASWeapon* NewWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponInfo.WeaponClass, FVector().ZeroVector, FRotator().ZeroRotator, SpawnParams);
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
		return MainWeapon;
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
	float SwapRate = 1.f;
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

void ASCharacter::SwapToNextAvailableWeapon(TEnumAsByte<EWeaponEquipType> CurrentWeaponEquipType)
{
	const TEnumAsByte<EWeaponEquipType> CurWeaponType = IsValid(CurrentWeapon) ? CurrentWeapon->GetWeaponEquipType() : CurrentWeaponEquipType;
	const int StartPos = WeaponEquipTypeList.IndexOfByKey(CurWeaponType);
	for(int i = StartPos; i < 2; ++i) //暂时只计算 主武器 副武器 这两种,如果已经是刀/拳头则无事发生，后续可扩充修改为循环切换
	{
		for(int j = i + 1; j < 3; ++j)
		{
			if(IsValid(GetWeaponByEquipType(WeaponEquipTypeList[j])))
			{
				StartSwapWeapon(WeaponEquipTypeList[j],true);
				return;
			}
		}
	}
}

void ASCharacter::DealSwapWeaponAttachment(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	if(!IsValid(GetWeaponByEquipType(WeaponEquipType)))
	{
		return;
	}
	
	if(IsValid(CurrentWeapon))
	{
		//若要切换位置的武器已经被装备为当前武器则无事发生
		if(CurrentWeapon == GetWeaponByEquipType(WeaponEquipType))
		{
			StopSwapWeapon(false);
			return;
		}
		CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetWeaponSocketName(CurrentWeapon->GetWeaponEquipType()));
	}
	//可以先将武器交换吸附这种纯表现的逻辑在双端都进行，但是实际更改CurrentWeapon赋值的行为只在服务器进行
	GetWeaponByEquipType(WeaponEquipType)->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponSocketName);
	
	if(!HasAuthority())
	{
		//客户端需等待服务器完成判断，在此之前本地虽结束了动画播放和计时器，但bIsSwapping状态应该仍为true，以禁止当前武器开火
		StopSwapWeapon(true);
		return;
	}

	if(HasAuthority())
	{
		CurrentWeapon = GetWeaponByEquipType(WeaponEquipType);
		//OnRep中会调用StopSwapWeapon(false)将bIsSwappingWeapon变为false
		OnRep_CurrentWeapon();
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
	//AIController里的UpdateControlRotation会调用此函数计算Controller的旋转，而且BP_AI继承于SCharacter，所以不希望AI调用
	if(!bIsAIPlayer && CameraComponent)
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
	StopSwapWeapon(false);
	
	//客户端或服务端都应在拾取武器时停止开火，但是生成新武器等操作仅在服务端执行
	DealPickUpWeapon(WeaponInfo);
}

void ASCharacter::DealPickUpWeapon_Implementation(FWeaponPickUpInfo WeaponInfo)
{
	ASWeapon*& WeaponToExchange = GetWeaponByEquipType(WeaponInfo.WeaponEquipType);
	FWeaponPickUpInfo InfoToBroadcast = IsValid(WeaponToExchange) ? WeaponToExchange->GetWeaponPickUpInfo() : FWeaponPickUpInfo();
	//把旧武器信息广播出去，可用于和地上可拾取武器的信息互换,广播出去空信息被PickUpWeapon接收到后，会导致PickUpWeapon被销毁
	OnExchangeWeapon.Broadcast(InfoToBroadcast);
	
	if(!WeaponToExchange)  //如果没装备对应位置的武器则直接拾取
	{
		WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
		if(!WeaponToExchange)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5 ,FColor::Red, TEXT("生成待拾取的武器失败!"));
			return;
		}
		if(!IsValid(CurrentWeapon) || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::MeleeWeapon || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::ThrowableWeapon)
		{
			StartSwapWeapon(WeaponInfo.WeaponEquipType, true);
		}
	}
	else
	{
		WeaponToExchange->Destroy();
		WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
		if(!WeaponToExchange)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5 ,FColor::Red, TEXT("生成待拾取的武器失败!"));
			return;
		}
		if(!IsValid(CurrentWeapon)) //如果当前武器类型和要拾取的类型相同，则销毁后CurrentWeapon会无效(不为nullptr)，此时执行交换逻辑正好
		{
			StartSwapWeapon(WeaponToExchange->GetWeaponEquipType(), true);
		}
	}
}

void ASCharacter::StartDropWeapon(bool ManuallyDiscard)
{
	if(!HasAuthority())
	{
		//客户端提前预测，以停止当前行动等待服务器同步，提升客户端在高延迟下的观感，不保持也不影响最终的数据同步
		if(!IsValid(CurrentWeapon) || (!ManuallyDiscard && !CurrentWeapon->GetWeaponCanDropDown())
		|| (ManuallyDiscard && !CurrentWeapon->GetWeaponCanManuallyDiscard()))
		{
			return;
		}
		StopFire();
		StopReload();
		StopSwapWeapon(false);
	}
	DealDropWeapon(ManuallyDiscard);  //Server方法
}

void ASCharacter::DealDropWeapon_Implementation(bool ManuallyDiscard)
{
	if(!IsValid(CurrentWeapon) || (!ManuallyDiscard && !CurrentWeapon->GetWeaponCanDropDown())
		|| (ManuallyDiscard && !CurrentWeapon->GetWeaponCanManuallyDiscard()))
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if(World)
	{
		TSubclassOf<APickUpWeapon> PickUpWeaponClass = APickUpWeapon::StaticClass();
		if(IsValid(CurrentWeapon) && CurrentWeapon->GetPickUpWeaponClass())
		{
			PickUpWeaponClass = CurrentWeapon->GetPickUpWeaponClass();
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
				FString::Printf(TEXT("CurrentWeapon的PickUpWeaponClass不存在！会导致生成的可拾取武器不会刷新信息！")));
		}

		//PickUpWeaponClass如果没有指定默认的SkeletalMesh(即默认为空指针)时,
		//会导致其实例在BeginPlay中设置完Mesh后立刻SetSimulatePhysics(true)时触发断言而游戏崩溃,
		//因为复制时Mesh还不存在而触发断言checkSlow(RootComponent->IsSimulatingPhysics())为false.
		APickUpWeapon* PickUpWeapon = World->SpawnActorDeferred<APickUpWeapon>(
			PickUpWeaponClass,
			FTransform(GetActorLocation() + GetActorForwardVector() *50 )
			);
		if(PickUpWeapon)
		{
			PickUpWeapon->SetOwner(this);
			PickUpWeapon->DefaultEditWeaponClass = nullptr;
			PickUpWeapon->bThrowAfterSpawn = ManuallyDiscard;
			
			PickUpWeapon->WeaponPickUpInfo = FWeaponPickUpInfo(
				this,
				CurrentWeapon->GetWeaponMeshComp()->SkeletalMesh,
				CurrentWeapon->GetClass(),
				CurrentWeapon->CurrentAmmoNum,
				CurrentWeapon->BackUpAmmoNum,
				CurrentWeapon->WeaponName,
				CurrentWeapon->GetWeaponEquipType(),
				true
				);
			//从C++中获取蓝图类
			const FString WidgetClassLoadPath = FString(TEXT("/Game/UI/WBP_ItemPickUpTip.WBP_ItemPickUpTip_C"));//蓝图一定要加_C这个后缀名
			UClass* Widget = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
			PickUpWeapon->WidgetComponent->SetWidgetClass(Widget);
			PickUpWeapon->bCanMeshDropOnTheGround = true;

			//主动扔掉的武器不能长按刷新为原武器(场景内摆放的或者什么道具点刷新的才可以)
			PickUpWeapon->bCanInteractKeyLongPress = false;

			//完成SpawnActor
			PickUpWeapon->FinishSpawning(FTransform(GetActorLocation() + GetActorForwardVector() *50));

			StopFire();
			StopReload();
			StopSwapWeapon(false);
			//掉落武器后销毁当前武器并自动切换为下一把
			const EWeaponEquipType CurType = CurrentWeapon->GetWeaponEquipType();
			CurrentWeapon->Destroy();
			GetWeaponByEquipType(CurType) = nullptr;
			SwapToNextAvailableWeapon(CurType);
		}
	}
}


void ASCharacter::StartFire()
{
	if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		//有子弹或者无限子弹才设置为开火状态，防止一按开火人物就瞬间面对正前方
		bool PreCheckRes = CurrentWeapon->CheckCanFire();
		CurrentWeapon->StartFire();
		if(IsLocallyControlled())
		{
			bIsFiring        = PreCheckRes;
			bIsFiringLocally = PreCheckRes;
		}
	}
}

void ASCharacter::StopFire()
{
	//if(bDisableGamePlayInput) return;
	if(CurrentWeapon && !GetIsSwappingWeapon())
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
		bool PreCheckRes = CurrentWeapon->CheckCanReload();
		CurrentWeapon->StartReload();
		if(IsLocallyControlled())
		{
			//正在换弹时再按换弹不会打断换单操作，且仍为换弹状态
			bIsReloading        = PreCheckRes || GetIsReloading();
			bIsReloadingLocally = PreCheckRes || GetIsReloading();
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

	if(!IsValid(GetWeaponByEquipType(NewWeaponEquipType)) || (CurrentWeapon == GetWeaponByEquipType(NewWeaponEquipType) && !GetIsSwappingWeapon()))
	{
		return;
	}
	
	//如果刚切枪到一半(此时还未完成交换)时又按了一下想切回原武器则允许交换，最终进入DealPlaySwapWeaponAnim的反播动画部分
	SwapWeapon(NewWeaponEquipType, Immediately);
	if(IsLocallyControlled())
	{
		bIsSwappingWeapon = !Immediately;
		bIsSwappingWeaponLocally = !Immediately;
	}
}

void ASCharacter::StopSwapWeapon(bool bWaitCurrentWeaponReplicated)
{
	GetWorldTimerManager().ClearTimer(SwapWeaponTimer);
	StopAnimMontage(CurrentSwapWeaponAnim);
	CurrentSwapWeaponAnim = nullptr;

	if(HasAuthority())
	{
		bIsSwappingWeapon = false;
		bIsSwappingWeaponLocally = false;
		return;
	}
	
	if(IsLocallyControlled())
	{
		//客户端需等待武器真正完成复制后才算完成交换(未实际交换时bWaitCurrentWeaponReplicated会填入false)
		bIsSwappingWeapon = bWaitCurrentWeaponReplicated;
		bIsSwappingWeaponLocally = bWaitCurrentWeaponReplicated;
	}
}

//外部操作不要直接调用这个函数，调用StartSwapWeapon()
void ASCharacter::SwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(bDisableGamePlayInput) return;
	
	//如果刚切枪到一半(此时还未完成交换)时又按了一下想切回原武器则允许交换，最终进入DealPlaySwapWeaponAnim的反播动画部分
	if(IsValid(CurrentWeapon) && CurrentWeapon == GetWeaponByEquipType(NewWeaponEquipType) && !GetIsSwappingWeapon())
	{
		return;
	}
	
	StopFire();
	StopReload();
	
	if(!HasAuthority())  //本地只处理一些表现效果，权威端在服务器
	{
		//分开处理是为了高延迟下客户端也能立刻响应一些表现
		ServerSwapWeapon(NewWeaponEquipType, Immediately);
		LocalSwapWeapon(NewWeaponEquipType, Immediately);
		return;
	}
	
	bIsSwappingWeapon = true;
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
	//因为双端分别执行Swap没用Multi，所以如果是服务器主控玩家，则需要同步播放Swap动画等表现到客户端
	if(HasAuthority() && IsLocallyControlled())
	{
		Multi_ClientSyncPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
	}
}

void ASCharacter::LocalSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	//本地会预测结果，所以把bIsSwappingWeapon = true;从DealPlaySwapWeaponAnim中剥离到Server
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
}

void ASCharacter::ServerSwapWeapon_Implementation(EWeaponEquipType NewWeaponEquipType, bool Immediately)
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
		if(GetIsSwappingWeapon() && CurrentSwapWeaponAnim && NewWeaponEquipType == CurrentWeapon->GetWeaponEquipType())
		{
			SwapAnim = CurrentSwapWeaponAnim;
			
			const UAnimInstance* AnimIns = GetMesh()->GetAnimInstance();
			MontagePlayTime = AnimIns->Montage_GetPosition(SwapAnim)/SwapAnimPlayRate;
			//播放速率变为复数即为反着播放(?)
			SwapAnimPlayRate = SwapAnimPlayRate * -1.f;
		}
		if(SwapAnim)
		{
			if(MontagePlayTime > 0)
			{
				CurrentSwapWeaponAnim = SwapAnim;
				
				PlayAnimMontage(CurrentSwapWeaponAnim, SwapAnimPlayRate);

				FTimerDelegate STD;  //SwapWeaponTimerDelegate
				//注意Lambda表达式的捕获方式如果为&则可能导致当前作用域内变量到生命周期之后再执行Timer时获取不到值而无法执行Lambda方法
				STD.BindWeakLambda(this,[=]
				{
					//无论正反播放动画都可以走进交换武器逻辑
					//只不过反向播放时CurrentWeapon还没有完成替换，会卡在DealSwapWeaponAttachment的判断那步，自然就不会完成切换
					DealSwapWeaponAttachment(NewWeaponEquipType);
				});
				
				float TimerRate = SwapAnimPlayRate > 0 ? 0.8 : 0.7;//反播动画时候稍微快点结束
				//GetWorldTimerManager().ClearTimer(SwapWeaponTimer);  //Set已存在的Timer会自动先Clear
				GetWorldTimerManager().SetTimer(
					SwapWeaponTimer,
					STD,
					MontagePlayTime* TimerRate,
					false);
			}
			else
			{
				const UEnum* TestEnumPtr = StaticEnum<EWeaponEquipType>();
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
					FString::Printf(TEXT("%s的 SwapAnimPlayRate < 0 ！"),*TestEnumPtr->GetDisplayNameTextByValue(static_cast<uint8>(NewWeaponEquipType)).ToString()));
			
				if(HasAuthority())  //客户端只播放动画
				{
					DealSwapWeaponAttachment(NewWeaponEquipType);
				}
			}
		}
		else
		{
			const UEnum* TestEnumPtr = StaticEnum<EWeaponEquipType>();
			GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red,
				FString::Printf(TEXT("Swap%sAnim蒙太奇不存在！"),*TestEnumPtr->GetDisplayNameTextByValue(static_cast<uint8>(NewWeaponEquipType)).ToString()));
				
			if(HasAuthority())  //客户端只播放动画
			{
				DealSwapWeaponAttachment(NewWeaponEquipType);
			}
		}
	}
}

void ASCharacter::Multi_ClientSyncPlaySwapWeaponAnim_Implementation(EWeaponEquipType NewWeaponEquipType, bool Immediately)
{
	if(HasAuthority() && IsLocallyControlled())
	{
		return;
	}
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
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

	//服务器调用OnRep时会立刻将bIsSwappingWeapon赋值为false，客户端需等待OnRep完成复制时才改为false以保持同步
	StopSwapWeapon(false);
	
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
	for(const auto& Type : WeaponEquipTypeList)
	{
		ASWeapon*& CurWeapon = GetWeaponByEquipType(Type);
		if(IsValid(CurWeapon) && CurWeapon->GetWeaponMeshComp())
		{
			CurWeapon->GetWeaponMeshComp()->SetVisibility(false);
			CurWeapon->ResetWeaponZoom();
		}
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

	DOREPLIFETIME(ASCharacter, bIsAIPlayer);
}

bool ASCharacter::NotEvent_NativeTest_Implementation()
{
	return true;
}
