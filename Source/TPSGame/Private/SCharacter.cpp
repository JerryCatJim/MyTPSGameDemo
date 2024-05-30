// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TPSGame/TPSGame.h"
//#include "Component/SHealthComponent.h"
//#include "Component/SBuffComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

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

	//设置Mesh的Transform
	GetMesh()->SetRelativeLocation(FVector(0,0,-87));
	GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
	
	//生命值组件初始化
	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));
	
	//Buff组件初始化
	BuffComponent = CreateDefaultSubobject<USBuffComponent>(TEXT("BuffComponent"));
	
	//背包组件初始化
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	
	ZoomedFOV = 65.f;
	ZoomInterpSpeed = 20.0f;
	
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
		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
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

	//当前视野范围
	const float TargetFOV = bIsAiming ? ZoomedFOV : DefaultFOV;

	//平滑插值
	const float NewFOV = FMath::FInterpTo(CameraComponent->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
	
	//设置开镜变焦效果
	CameraComponent->SetFieldOfView(NewFOV);

	//根据是否开镜设置人物的移动类型
	SetPlayerControllerRotation();
}

void ASCharacter::MoveForward(float Value)
{
	FVector ForwardV = FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetScaledAxis(EAxis::X);
	AddMovementInput(ForwardV, Value);
	//AddMovementInput(FRotator(0,GetControlRotation().Yaw,0).Vector(), Value);
	//AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacter::MoveRight(float Value)
{
	FVector RightV = FRotationMatrix(FRotator(0,GetControlRotation().Yaw,0)).GetScaledAxis(EAxis::Y);
	AddMovementInput(RightV, Value);
	//AddMovementInput(GetActorRightVector() * Value);  //bUseControllerRotationYaw为false且GetCharacterMovement()->bOrientRotationToMovement时，会导致原地打转
}

void ASCharacter::BeginJump()
{
	Jump();
}

void ASCharacter::EndJump()
{
	StopJumping();
}

void ASCharacter::BeginCrouch()
{
	Crouch();
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
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
	//绑定轴映射输入
	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);
}

//SWeapon.cpp中 Fire函数会让Owner(即该文件)调用GetActorEyesViewPoint,下面函数重写了方法，使眼部位置变为摄像机位置
//Character继承于Pawn, Pawn.cpp中 调用GetActorEyesViewPoint 会调用GetPawnViewLocation和GetViewRotation获得值
FVector ASCharacter::GetPawnViewLocation() const
{
	//判断摄像机组件是否为空，然后返回该组件
	if(CameraComponent)
	{
		return CameraComponent->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}


//将开镜行为发送到服务器然后同步
void ASCharacter::SetZoomFOV_Implementation()
{
	//空手时不能开镜
	bIsAiming = bIsUsingWeapon;
}

void ASCharacter::ResetZoomFOV_Implementation()
{
	bIsAiming = false;
}

//将射击行为发送到服务器然后同步
void ASCharacter::SetIsFiring_Implementation(bool IsFiring)
{
	bIsFiring = IsFiring;
}

void ASCharacter::StartFire()
{
	if(CurrentWeapon)
	{
		//有子弹或者无限子弹才设置为开火状态，防止一按开火人物就瞬间面对正前方
		//SetIsFiring(CurrentWeapon->CheckCanFire());    //挪到Sweapon.cpp的StartFire()中了
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire()
{
	if(CurrentWeapon)
	{
		//SetIsFiring(false);    //挪到Sweapon.cpp的StopFire()中了
		CurrentWeapon->StopFire();
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
		bDied = true;
		if(HasAuthority())
		{
			OnRep_Died();
			OnPlayerDead.Broadcast(InstigatedBy, DamageCauser, DamageType);
		}
	}
}

void ASCharacter::OnRep_CurrentWeapon()
{
	OnCurrentWeaponChanged.Broadcast();
}

void ASCharacter::OnRep_Died()
{
	//角色死亡
	GetMovementComponent()->UNavMovementComponent::StopMovementImmediately();  //无法移动  4.27以上用GetMovementComponent()->StopMovement会出错(？)
	GetCharacterMovement()->DisableMovement();
		
	//获取胶囊体碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	//死亡后解除控制器权限，销毁控制器(生成UI时不指定Owner会报错)
	//DetachFromControllerPendingDestroy();
	//SetLifeSpan(10.f);  //10秒后自动销毁
	//CurrentWeapon->SetLifeSpan(10.f);
	
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
	return InventoryComponent;
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
}

bool ASCharacter::NotEvent_NativeTest_Implementation()
{
	return true;
}

int ASCharacter::GetIndex()
{
	//return Index++;
	UE_LOG(LogTemp, Log, TEXT("C++Index"));
	return 0;
}

int ASCharacter::testmyfunc(int param1, int& param2)
{
	return param1, param2;
}
 void ASCharacter::testtest()
 {
	int temp  =  100;
	int rtv1(0), rtv2;
	rtv1, rtv2 = testmyfunc(1, temp);
	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("rtv1和rtv2分别等于 %d %d"), rtv1, rtv2));
 }
