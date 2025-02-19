// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemBase.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Net/UnrealNetwork.h"
#include "TPSGameType/CustomCollisionType.h"


// Sets default values
AInventoryItemBase::AInventoryItemBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	//SetReplicateMovement(true);
	SetReplicatingMovement(true);
	
	if(!ItemSlot.ItemClass)
	{
		ItemSlot.ItemClass = StaticClass();
	}

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	//MeshComponent->SetupAttachment(RootComponent);
	SetRootComponent(MeshComponent);  //第一个Component需要SetRootComponent而不是AttachToRoot, 否则其他组件AttachToRoot时拖入场景后的默认相对坐标会错乱，且蓝图中看不到Transform的Location和Rotation
	
	//声明创建UWidgetComponent需要在build.cs文件中添加UMG模块！！！
	TextWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("TextWidget"));
	TextWidget->SetupAttachment(RootComponent);
	TextWidget->SetRelativeLocation(FVector(0,0,50));
	TextWidget->SetRelativeRotation(FRotator(0,0,0));
	TextWidget->SetWidgetSpace(EWidgetSpace::Screen);
	
	if(ItemSlot.Mesh)
	{
		MeshComponent->SetStaticMesh(ItemSlot.Mesh);
	}
	else
	{
		//auto DefaultMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("StaticMesh'/Game/InventorySystem/InventoryItemCube.InventoryItemCube'"));
		if(DefaultMesh.Succeeded())
		{
			MeshComponent->SetStaticMesh(DefaultMesh.Object);
			/*static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(TEXT("Material'/Game/InventorySystem/ItemTestFocusMaterial.ItemTestFocusMaterial'"));
			if(DefaultMaterial.Succeeded())
			{
				MeshComponent->SetMaterial(0, DefaultMaterial.Object);
			}*/
		}
	}
	if(MeshComponent->GetStaticMesh())
	{
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		MeshComponent->SetCollisionObjectType(Collision_InventoryItem);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	}
}

// Called when the game starts or when spawned
void AInventoryItemBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
/*void AInventoryItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}*/

void AInventoryItemBase::OnRep_TestValue()
{
	UE_LOG(LogTemp, Log, TEXT("客户端的TestValue : %d"), TestValue)
	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Blue, FString::Printf(TEXT("客户端的TestValue : %d"),TestValue));
}

void AInventoryItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	DOREPLIFETIME(AInventoryItemBase, TestValue);
	DOREPLIFETIME(AInventoryItemBase, ItemSlot);
}

