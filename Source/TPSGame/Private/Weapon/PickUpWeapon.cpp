// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/PickUpWeapon.h"

#include "SCharacter.h"
#include "../TPSGame.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APickUpWeapon::APickUpWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	SetReplicatingMovement(true);
	
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));

	SetRootComponent(CapsuleComponent);
	MeshComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetupAttachment(RootComponent);
	
	CapsuleComponent->SetCapsuleRadius(44);
	CapsuleComponent->SetCapsuleHalfHeight(88);

	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

	//防止武器被踢走撞走等情况
	MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
}

// Called when the game starts or when spawned
void APickUpWeapon::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;  //要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空
	
	ASWeapon* CurWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClass, FVector().ZeroVector, FRotator().ZeroRotator, SpawnParams);
	WeaponPickUpInfo = CurWeapon->GetWeaponPickUpInfo();
	//记录原始武器信息
	OriginalWeaponInfo = WeaponPickUpInfo;

	CurWeapon->Destroy();

	//从C++中获取蓝图类
	const FString WidgetClassLoadPath = FString(TEXT("/Game/UI/WBP_ItemPickUpTip.WBP_ItemPickUpTip_C"));//蓝图一定要加_C这个后缀名
	UClass* Widget = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
	WidgetComponent->SetWidgetClass(Widget);
	
	CapsuleComponent->SetCollisionResponseToChannel(Collision_Weapon, ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(Collision_Projectile, ECR_Ignore);

	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &APickUpWeapon::OnCapsuleComponentBeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &APickUpWeapon::OnCapsuleComponentEndOverlap);

	if(IsValid(WeaponPickUpInfo.WeaponMesh))
	{
		MeshComponent->SetSkeletalMesh(WeaponPickUpInfo.WeaponMesh);
		if(MeshComponent->SkeletalMesh)
		{
			//让武器模拟物理，达到可以落在地上的效果
			MeshComponent->SetSimulatePhysics(bCanMeshDropOnTheGround);
		}
	}
	
	ShowTipWidget(false);
}

// Called every frame
void APickUpWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APickUpWeapon::OnInteractKeyUp()
{
	//TryPickUpWeapon会触发Character的Server端的广播回到这个Actor，不用手动触发其对应事件广播的Server版
	TryPickUpWeapon();
}
void APickUpWeapon::OnInteractKeyDown()
{
	TryPickUpWeapon();
}
void APickUpWeapon::OnInteractKeyLongPressed()
{
	if(!HasAuthority())
	{
		if(LastOverlapPlayer)
		{
			LastOverlapPlayer->Server_OnInteractKeyLongPressed();
		}
		return;
	}
	//人物的Interact事件未限定为仅Server端，若以客户端触发了广播，由于此Actor为ROLE_SimulatedProxy，
	//从客户端发送的Server版RPC调用会被丢弃，Multicast也仅会本地调用，所以需手动触发SCharacter的Server版事件广播以完成此Actor的Server端逻辑响应
	if(bCanInteractKeyLongPress)
	{
		//进入此处时已从SCharacter的Server广播函数来到了Server端执行
		Server_ResetPickUpWeaponInfo(OriginalWeaponInfo);
	}
}

void APickUpWeapon::TryPickUpWeapon()
{
	if(LastOverlapPlayer)
	{
		LastOverlapPlayer->PickUpWeapon(WeaponPickUpInfo);  //PickUpWeapon会在Server端执行OnPickUpWeapon的广播回到这个Actor
	}
}

void APickUpWeapon::Server_ResetPickUpWeaponInfo_Implementation(FWeaponPickUpInfo NewInfo)
{
	WeaponPickUpInfo = NewInfo;
	if(HasAuthority())
	{
		OnRep_WeaponPickUpInfo();
	}
}

void APickUpWeapon::OnRep_WeaponPickUpInfo()
{
	//拾取武器后 地上武器的提示文字名字和Mesh没改为被换下来的武器，手动刷新一下
	if(IsValid(WeaponPickUpInfo.WeaponMesh))
	{
		MeshComponent->SetSkeletalMesh(WeaponPickUpInfo.WeaponMesh);
	}
	else
	{
		MeshComponent->SetSkeletalMesh(OriginalWeaponInfo.WeaponMesh);
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, TEXT("当前角色替换下来的武器为空，将可拾取武器刷新为原始武器"));
	}
	ShowTipWidgetOnOwningClient();
}

void APickUpWeapon::OnCapsuleComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//重叠时角色死亡会仅在服务端再触发一次OverlapBegin，然后LastPlayer转换失败(???)，然后OverlapEnd，所以在这限制一下
	if(!LastOverlapPlayer) LastOverlapPlayer = Cast<ASCharacter>(OtherActor);
	if(LastOverlapPlayer)
	{
		//防止同时间有多个可拾取武器与人物重叠
		if(LastOverlapPlayer->bHasBeenOverlappedWithPickUpWeapon == false)
		{
			LastOverlapPlayer->bHasBeenOverlappedWithPickUpWeapon = true;
			bIsThisTheOverlapOne = true;
		}
		else
		{
			return;
		}
		
		//对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件，用以区分
		if(bCanInteractKeyLongPress)
		{
			LastOverlapPlayer->OnInteractKeyUp.AddDynamic(this, &APickUpWeapon::OnInteractKeyUp);
			LastOverlapPlayer->OnInteractKeyLongPressed.AddDynamic(this, &APickUpWeapon::OnInteractKeyLongPressed);
		}
		else
		{
			LastOverlapPlayer->OnInteractKeyDown.AddDynamic(this, &APickUpWeapon::OnInteractKeyDown);
		}
		
		//OnPickUpWeapon在SCharacter中仅服务端广播
		LastOverlapPlayer->OnExchangeWeapon.AddDynamic(this, &APickUpWeapon::Server_ResetPickUpWeaponInfo);
		//角色在拾取范围内死亡时也关闭文字显示
		LastOverlapPlayer->OnPlayerDead.AddDynamic(this, &APickUpWeapon::OnPlayerDead);

		ShowTipWidgetOnOwningClient();
	}
}

void APickUpWeapon::OnCapsuleComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(!Cast<ASCharacter>(OtherActor)) return;
	//角色死亡时触发倒地动画，会仅在服务端连续触发两次EndOverlap(???)，不在客户端触发，为什么呢
	//角色重生时销毁了原先的人物模型，也会触发一次双端的EndOverlap，但没什么影响了
	if(LastOverlapPlayer)
	{
		if(bIsThisTheOverlapOne == false)
		{
			return;
		}

		//对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件，用以区分
		if(bCanInteractKeyLongPress)
		{
			LastOverlapPlayer->OnInteractKeyUp.RemoveDynamic(this, &APickUpWeapon::OnInteractKeyUp);
			LastOverlapPlayer->OnInteractKeyLongPressed.RemoveDynamic(this, &APickUpWeapon::OnInteractKeyLongPressed);
		}
		else
		{
			LastOverlapPlayer->OnInteractKeyDown.RemoveDynamic(this, &APickUpWeapon::OnInteractKeyDown);
		}
		LastOverlapPlayer->OnExchangeWeapon.RemoveDynamic(this, &APickUpWeapon::Server_ResetPickUpWeaponInfo);
		LastOverlapPlayer->OnPlayerDead.RemoveDynamic(this, &APickUpWeapon::OnPlayerDead);
		ShowTipWidget(false);
		LastOverlapPlayer->bHasBeenOverlappedWithPickUpWeapon = false;
		
		//GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("离开了 %d"), HasAuthority()));
	}
	/*else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("离开了但是角色不存在 %d"), HasAuthority()));
	}*/
	LastOverlapPlayer = nullptr;
	bIsThisTheOverlapOne = false;
}

void APickUpWeapon::ShowTipWidget_Implementation(bool bIsVisible)
{
	if(!IsValid(WidgetComponent->GetWidget())) return;
	if(bIsVisible)
	{
		WidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		WidgetComponent->GetWidget()->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void APickUpWeapon::ShowTipWidgetOnOwningClient()
{
	if(LastOverlapPlayer && LastOverlapPlayer->GetController())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, FString::Printf(TEXT("本地展示判断处进来了 %d"), HasAuthority()));
		ShowTipWidget(LastOverlapPlayer->GetController() == UGameplayStatics::GetPlayerController(this,0));
	}
	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Blue, FString::Printf(TEXT("本地展示判断处没进来，控制器不一样 %d"), HasAuthority()));
		ShowTipWidget(false);
	}
}

void APickUpWeapon::OnPlayerDead(AController* InstigatedBy, AActor* DamageCauser,const UDamageType* DamageType)
{
	//角色死亡时触发倒地动画，没触发客户端的EndOverlap(为什么呢???)，所以没Remove委托，客户端就走到了这里(服务端在EndOverlap就Remove了，走不到这里)
	if(LastOverlapPlayer)
	{
		ShowTipWidget(false);
		
		//对于不需要长按的互动对象则只绑定KeyDown事件，否则只绑定KeyUp和LongPress事件，用以区分
		if(bCanInteractKeyLongPress)
		{
			LastOverlapPlayer->OnInteractKeyUp.RemoveDynamic(this, &APickUpWeapon::OnInteractKeyUp);
			LastOverlapPlayer->OnInteractKeyLongPressed.RemoveDynamic(this, &APickUpWeapon::OnInteractKeyLongPressed);
		}
		else
		{
			LastOverlapPlayer->OnInteractKeyDown.RemoveDynamic(this, &APickUpWeapon::OnInteractKeyDown);
		}
		LastOverlapPlayer->OnExchangeWeapon.RemoveDynamic(this, &APickUpWeapon::Server_ResetPickUpWeaponInfo);
		LastOverlapPlayer->OnPlayerDead.RemoveDynamic(this, &APickUpWeapon::OnPlayerDead);
		
		LastOverlapPlayer = nullptr;
		bIsThisTheOverlapOne = false;
		
		//GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("死了 %d"), HasAuthority()));
	}/*
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::Printf(TEXT("死了但是Player不存在 %d"), HasAuthority()));
	}*/
}

void APickUpWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(APickUpWeapon, OriginalWeaponInfo);
	DOREPLIFETIME(APickUpWeapon, WeaponPickUpInfo);
	DOREPLIFETIME(APickUpWeapon, bCanMeshDropOnTheGround);
	DOREPLIFETIME(APickUpWeapon, bCanInteractKeyLongPress);
}