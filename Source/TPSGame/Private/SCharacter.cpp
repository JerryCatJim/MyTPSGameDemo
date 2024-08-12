// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TPSGame/TPSGame.h"
//#include "Component/SHealthComponent.h"
//#include "Component/SBuffComponent.h"
#include "TPSPlayerController.h"
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
	
	WeaponSocketName = "WeaponSocket";

	bDied = false;
	AimOffset_Y = 0;
	AimOffset_Z = 0;
	bIsAiming = false;
	bIsFiring = false;
	bIsUsingWeapon = true;
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

	//如果控制权在服务器Server(相对Client)则执行下列代码
	if(HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		//要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空
		SpawnParams.Owner = this;
	
		//生成默认武器
		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(CurrentWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if(CurrentWeapon)
		{
			//CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
			//C++中的服务器不会自动调用OnRep函数，需要手动调用
			OnRep_CurrentWeapon();
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
	if(bDisableGamePlayInput) return;
	//由于交互按钮会对不同物体产生不同反馈，应将这一行为广播出去，由要产生互动的一端绑定委托，然后收到广播后自行编写响应行为
	OnInteractKeyDown.Broadcast();
}

void ASCharacter::InteractKeyReleased()//_Implementation()
{
	if(bDisableGamePlayInput) return;
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
	
	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::SetZoomFOV);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::ResetZoomFOV);

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
void ASCharacter::SetZoomFOV_Implementation()
{
	if(bDisableGamePlayInput) return;
	//空手时不能开镜
	bIsAiming = bIsUsingWeapon;
}

void ASCharacter::ResetZoomFOV_Implementation()
{
	if(bDisableGamePlayInput) return;
	bIsAiming = false;
}

//将射击行为发送到服务器然后同步
void ASCharacter::SetIsFiring_Implementation(bool IsFiring)
{
	bIsFiring = IsFiring;
}

void ASCharacter::PickUpWeapon_Implementation(FWeaponPickUpInfo WeaponInfo)
{
	const FWeaponPickUpInfo LastWeaponInfo = GetWeaponPickUpInfo();
	StopReload();
	StopFire();
	
	//把旧武器信息广播出去，可用于和地上可拾取武器的信息互换
	OnPickUpWeapon.Broadcast(LastWeaponInfo);
	
	CurrentWeaponClass = WeaponInfo.WeaponClass;
	
	CurrentWeapon->Destroy(true);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(
		CurrentWeaponClass,
		FTransform(),
		SpawnParameters
		);
	if(CurrentWeapon)
	{
		CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		CurrentWeapon->RefreshWeaponInfo(WeaponInfo);
		//C++中的服务器不会自动调用OnRep函数，需要手动调用
		OnRep_CurrentWeapon();
	}
}

void ASCharacter::DropWeapon_Implementation()
{
	if(!HasAuthority())  //客户端不要生成武器，等待服务端生成后复制
	{
		return;
	}
	if(!CurrentWeapon)
	{
		return;  //没有武器也不生成
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
				CurrentWeaponClass,
				CurrentWeapon->CurrentAmmoNum,
				CurrentWeapon->BackUpAmmoNum,
				CurrentWeapon->WeaponName
				);
			//从C++中获取蓝图类
			const FString WidgetClassLoadPath = FString(TEXT("/Game/UI/WBP_ItemPickUpTip.WBP_ItemPickUpTip_C"));//蓝图一定要加_C这个后缀名
			UClass* Widget = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
			PickUpWeapon->WidgetComponent->SetWidgetClass(Widget);
			PickUpWeapon->bCanMeshDropOnTheGround = true;
			
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
		//SetIsFiring(CurrentWeapon->CheckCanFire());    //挪到Sweapon.cpp的StartFire()中了
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire()
{
	//if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		//SetIsFiring(false);    //挪到Sweapon.cpp的StopFire()中了
		CurrentWeapon->StopFire();
	}
}

void ASCharacter::StartReload()
{
	if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		CurrentWeapon->Reload(false);
	}
}

void ASCharacter::StopReload()
{
	//if(bDisableGamePlayInput) return;
	if(CurrentWeapon)
	{
		CurrentWeapon->StopReload(true);
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
	//角色死亡
	GetMovementComponent()->UNavMovementComponent::StopMovementImmediately();  //无法移动  4.27以上用GetMovementComponent()->StopMovement会出错(？)
	GetCharacterMovement()->DisableMovement();
	bDisableGamePlayInput = true;
	StopFire();

	if(HasAuthority())
	{
		DropWeapon();  //死亡后掉落武器,然后隐藏手中武器
	}
	if(CurrentWeapon && CurrentWeapon->GetWeaponMeshComp())
	{
		CurrentWeapon->GetWeaponMeshComp()->SetVisibility(false);
	}
	ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
	if(MyController)
	{
		MyController->RemoveCrossHair();
	}
		
	//获取胶囊体碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GetWorldTimerManager().SetTimer(FPlayerRespawnTimerHandle,
		[this]()->void
		{
			ATPSPlayerController* MyController = Cast<ATPSPlayerController>(GetController());
			if(MyController)
			{
				MyController->RequestRespawn();
			}
		},
		RespawnCount,
		false);
}

USHealthComponent* ASCharacter::GetHealthComponent()
{
	return HealthComponent;
}

USBuffComponent* ASCharacter::GetBuffComponent()
{
	return BuffComponent;
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

bool ASCharacter::GetIsDied()
{
	return bDied;
}

bool ASCharacter::GetIsAiming()
{
	return bIsAiming;
}

bool ASCharacter::GetIsFiring()
{
	return bIsFiring;
}

bool ASCharacter::GetIsReloading()
{
	if(!IsValid(CurrentWeapon))
	{
		return false;
	}
	return CurrentWeapon->bIsReloading;
}

float ASCharacter::GetAimOffset_Y()
{
	return AimOffset_Y;
}

float ASCharacter::GetAimOffset_Z()
{
	return AimOffset_Z;
}

float ASCharacter::GetSpringArmLength()
{
	return SpringArmComponent ? SpringArmComponent->TargetArmLength : 0 ;
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
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, bIsAiming);
	DOREPLIFETIME(ASCharacter, bIsFiring);
	DOREPLIFETIME(ASCharacter, AimOffset_Y);
	DOREPLIFETIME(ASCharacter, AimOffset_Z);
	DOREPLIFETIME(ASCharacter, bDisableGamePlayInput);
}

bool ASCharacter::NotEvent_NativeTest_Implementation()
{
	return true;
}
