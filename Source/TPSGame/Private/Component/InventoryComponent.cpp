// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/InventoryComponent.h"

#include "DrawDebugHelpers.h"
#include "InventoryItemBase.h"
#include "Kismet/GameplayStatics.h"
#include "Component/InventoryDealUseItemClass.h"

#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
	//开启网络复制，将该实体从服务器端Server复制到客户端Client
	SetIsReplicatedByDefault(true);

	InventorySize = 30;
	TraceLength = 300;
	
	InitInventoryComponent();
	
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	MyController = Cast<APlayerController>(GetOwner());
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FocusOnPickUpItem();
}

void UInventoryComponent::InitInventoryComponent()
{
	ItemList.Init(EmptyItem, InventorySize);
	for(int i = 0; i < ItemList.Num(); ++i)
	{
		ItemList[i].ItemIndex = i;
	}
	CurrentUsedSize = 0;
}

void UInventoryComponent::TrySetOwnerPawn()
{
	if(MyController)
	{
		MyOwnerPawn = MyController->GetPawn();
	}
}

int UInventoryComponent::GetCurrentSize() const
{
	return CurrentUsedSize;
}

int UInventoryComponent::FindEmptyPos() const
{
	for(TArray<FMyItem>::TConstIterator It = ItemList.CreateConstIterator(); It; ++It)
	{
		if(It->ItemID == -1)
		{
			return It.GetIndex();
		}
	}
	return -1;
}

int UInventoryComponent::FindFirstFilledPos() const
{
	for(TArray<FMyItem>::TConstIterator It = ItemList.CreateConstIterator(); It; ++It)
	{
		if(It->ItemID != -1)
		{
			return It.GetIndex();
		}
	}
	return -1;
}


bool UInventoryComponent::CheckIsValidIndex(int Index) const
{
	return	Index >= 0 && Index <= InventorySize - 1;
}

void UInventoryComponent::InputAddItem()
{
	if(!CurrentFocusItem)
	{
		return;
	}
	AddItem(CurrentFocusItem->ItemSlot, true);
}

void UInventoryComponent::AddItem_Implementation(FMyItem ItemToAdd, bool IsPickUp)
{
	//Server函数，不能返回值，也不能传入非Const引用
	const bool DealResult = DealAddItem(ItemToAdd, IsPickUp);
	
	//服务端手动触发刷新UI
	if(DealResult && MyController->HasAuthority())
	{
		OnRep_ItemList();
	}
	
	PostAddItem(ItemToAdd, IsPickUp);
}

bool UInventoryComponent::DealAddItem(FMyItem ItemToAdd, bool IsPickUp)
{
	if(IsPickUp)
	{
		DestroyCurrentFocusItem();
	}

	LastChangedItemSlot = ItemToAdd;
	
	if(ItemToAdd.CanStack)
	{
		//有相同物品
		for(FMyItem& ArrayItem : ItemList)
		{
			if(ArrayItem.ItemID == ItemToAdd.ItemID)
			{
				ArrayItem.ItemQuantity += ItemToAdd.ItemQuantity;
				//记录物品修改Index
				LastChangedItemSlot.ItemIndex = ArrayItem.ItemIndex;
				return true;
			}
		}
		//没有相同物品
		const int FirstEmptyPos = FindEmptyPos();
		if(FirstEmptyPos != -1)
		{
			ItemList[FirstEmptyPos] = ItemToAdd;
			//Index应为目前格子的Index，别忘了修改
			ItemList[FirstEmptyPos].ItemIndex = FirstEmptyPos;
			
			//记录物品修改Index
			LastChangedItemSlot.ItemIndex = FirstEmptyPos;
			return true;
		}
		return false;
	}
	else
	{
		if(InventorySize - GetCurrentSize() < ItemToAdd.ItemQuantity)
		{
			//空间不足
			return false;
		}
		
		//一次性加入多个不可堆叠的物品时，记录物品Index为第一个空格子
		LastChangedItemSlot.ItemIndex = FindEmptyPos();
		LastChangedItemSlot.ItemQuantity = -1;  //物品数量为-1意思为UI全刷新
		
		for(int i = 0; i < ItemToAdd.ItemQuantity; ++i)
		{
			const int TempPos = FindEmptyPos();
			ItemList[TempPos] = ItemToAdd;
			ItemList[TempPos].ItemQuantity = 1;
			//Index应为目前格子的Index，别忘了修改
			ItemList[TempPos].ItemIndex = TempPos;
		}
		return true;
	}
}

void UInventoryComponent::DestroyCurrentFocusItem_Implementation()
{
	if(CurrentFocusItem)
	{
		//被拾取的物品在服务端被删除，又因为InventoryItemBase开启了网络复制，所以客户端的物品也被同步删除
		CurrentFocusItem->Destroy();
		CurrentFocusItem = nullptr;
	}
}

bool UInventoryComponent::PostAddItem_Implementation(FMyItem ItemToAdd, bool IsPickUp)
{
	return true;
}

void UInventoryComponent::InputRemoveItem(int ArrayIndex, int RemoveQuantity)
{
	RemoveItem(ArrayIndex, RemoveQuantity);
}

void UInventoryComponent::RemoveItem_Implementation(int ArrayIndex, int RemoveQuantity, bool IsThrow)
{
	//Server函数，不能返回值，也不能传入非Const引用
	const bool DealResult = DealRemoveItem(ArrayIndex, RemoveQuantity, IsThrow);

	//服务端手动触发刷新UI
	if(DealResult && MyController->HasAuthority())
	{
		OnRep_ItemList();
	}
	
	PostRemoveItem(ArrayIndex, RemoveQuantity);
}

bool UInventoryComponent::DealRemoveItem(int ArrayIndex, int RemoveQuantity, bool IsThrow)
{
	if(!CheckIsValidIndex(ArrayIndex) || ItemList[ArrayIndex].ItemID == -1 || ItemList[ArrayIndex].ItemQuantity < RemoveQuantity || RemoveQuantity <= 0)
	{
		return false;
	}

	FMyItem RemovedItem = ItemList[ArrayIndex];
	RemovedItem.ItemQuantity = RemoveQuantity;
	LastChangedItemSlot = RemovedItem;
	
	ItemList[ArrayIndex].ItemQuantity -= RemoveQuantity;
	if(ItemList[ArrayIndex].ItemQuantity == 0)
	{
		ItemList[ArrayIndex] = EmptyItem;
		ItemList[ArrayIndex].ItemIndex = ArrayIndex;
	}
	if(IsThrow)
	{
		SpawnCurrentThrowItem(RemovedItem);
	}
	return true;
}

void UInventoryComponent::SpawnCurrentThrowItem_Implementation(FMyItem ItemToRemove)
{
	//todo 丢弃可能分为在城镇中丢弃(直接消失),或者在副本中丢弃(丢到地上可以再捡起来) 等情况

	if(!MyOwnerPawn)
	{
		return;
	}
	
	//本函数在服务器运行，本地客户端不要重复生成，等待服务端网络复制
	const FTransform PawnTransform = FTransform(MyOwnerPawn->GetActorLocation() + MyOwnerPawn->GetActorForwardVector() * ThrowForwardDistance);
	//AActor* SpawnedActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(MyOwnerPawn->GetWorld(), ItemToRemove.ItemClass, PawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn); //第五个参数是Owner,希望用Instigator
	AInventoryItemBase* SpawnedActor = MyOwnerPawn->GetWorld()->SpawnActorDeferred<AInventoryItemBase>(ItemToRemove.ItemClass, PawnTransform, nullptr, MyOwnerPawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if(SpawnedActor)
	{
		ThrownItem = Cast<AInventoryItemBase>(SpawnedActor);
		ThrownItem->ItemSlot = ItemToRemove;
		UGameplayStatics::FinishSpawningActor(SpawnedActor, PawnTransform);
		OnRep_ThrownItem();
	}
}

void UInventoryComponent::OnRep_ThrownItem()
{
	if(ThrownItem)
	{
		const FVector ThrowVelocity = (ThrownItem->GetInstigator()->GetActorForwardVector() + FVector(0, 0, 1)) * ThrowPowerSpeed;
		ThrownItem->MeshComponent->SetPhysicsLinearVelocity(ThrowVelocity);
	}
}

void UInventoryComponent::OnRep_ItemList()
{
	OnItemListChanged.Broadcast();
}


bool UInventoryComponent::PostRemoveItem_Implementation(int ArrayIndex, int RemoveQuantity)
{
	return true;
}

void UInventoryComponent::InputSwapItem(int FirstIndex, int SecondIndex)
{
	SwapItem(FirstIndex, SecondIndex);
}

void UInventoryComponent::SwapItem_Implementation(int FirstIndex, int SecondIndex)
{
	const bool DealResult = DealSwapItem(FirstIndex, SecondIndex);

	//服务端手动触发刷新UI
	if(DealResult && MyController->HasAuthority())
	{
		OnRep_ItemList();
	}
}

bool UInventoryComponent::DealSwapItem(int FirstIndex, int SecondIndex)
{
	if(!CheckIsValidIndex(FirstIndex) || !CheckIsValidIndex(SecondIndex))
	{
		return false;
	}

	LastChangedItemSlot = ItemList[FirstIndex];
	
	ItemList.Swap(FirstIndex, SecondIndex);  //会触发一次OnRep_ItemList
	
	ItemList[FirstIndex].ItemIndex = FirstIndex;
	ItemList[SecondIndex].ItemIndex = SecondIndex;
	LastChangedItemSlot.ItemQuantity = -1;  //ItemQuantity = -1表示刷新所有格子
	return true;
}

void UInventoryComponent::InputUseItem(int ArrayIndex, int ItemQuantity)
{
	UseItem(ArrayIndex, ItemQuantity);
}

void UInventoryComponent::UseItem_Implementation(int ArrayIndex, int ItemQuantity)
{
	const bool DealResult = DealUseItem(ArrayIndex, ItemQuantity);
	
	//服务端手动触发刷新UI
	if(DealResult && MyController->HasAuthority())
	{
		OnRep_ItemList();
	}
}

bool UInventoryComponent::DealUseItem(int ArrayIndex, int ItemQuantity)
{
	if(ItemList[ArrayIndex].ItemID <= 0 || !CheckIsValidIndex(ItemList[ArrayIndex].ItemIndex))
	{
		return false;
	}
	
	bool DealResult = false;
	int RealUsedItemQuantity = 0;
	for(int i = 0; i < ItemQuantity; ++i)
	{
		const bool TempRes = InventoryDealUseItemClass::DealUseItemByItemID(ItemList[ArrayIndex].ItemID, ItemQuantity, MyController);
		DealResult = DealResult == false ? TempRes : DealResult;  //只要至少成功使用过一个道具，就认为是使用成功，返回到UseItem中刷新UI
		RealUsedItemQuantity += TempRes ? 1 : 0;
	}
	if(DealResult)
	{
		//LastChangedItemSlot = DealResult ? ItemList[ArrayIndex] : LastChangedItemSlot;  //RemoveItem()中会记录LastChangedItemSlot
		RemoveItem(ArrayIndex, RealUsedItemQuantity, false);
	}
	return DealResult;
}

void UInventoryComponent::FocusOnPickUpItem()
{
	if(!MyOwnerPawn)
	{
		TrySetOwnerPawn();
		return;
	}
	
	FHitResult HitResult;
	FVector EyeLocation;
	FRotator EyeRotation;
	//SCharacter.cpp中重写了Pawn.cpp的GetPawnViewLocation().以获取CameraComponent的位置而不是人物Pawn的位置
	MyOwnerPawn->GetActorEyesViewPoint(EyeLocation,EyeRotation);

	FVector TraceDirection = EyeRotation.Vector()*TraceLength;
	FVector TraceEnd = EyeLocation + TraceDirection;

	//碰撞查询
	FCollisionQueryParams QueryParams;
	//忽略武器自身和持有者的碰撞
	QueryParams.AddIgnoredActor(MyOwnerPawn);
	QueryParams.bTraceComplex = true;  //启用复杂碰撞检测，更精确
	//QueryParams.bReturnPhysicalMaterial = true;  //物理查询为真，否则不会返回自定义材质
	
    bool bIsTraceHit = GetWorld()->LineTraceSingleByObjectType(HitResult, EyeLocation, TraceEnd, ECC_WorldDynamic, QueryParams);
	if(bIsTraceHit)
	{
		AInventoryItemBase* TempFocusItem = Cast<AInventoryItemBase>(HitResult.GetActor());//Actor.Get();
		if(TempFocusItem)  //物品被注视
		{
			CurrentFocusItem = TempFocusItem;
			//别的玩家注视时也会触发广播，所以传入注视者加以区分
			CurrentFocusItem->OnItemFocused.Broadcast(MyController);
		}
		else if(CurrentFocusItem)  //物品离开注视
		{
			CurrentFocusItem->OnItemLeaveFocused.Broadcast(MyController);
			CurrentFocusItem = nullptr;
		}
	}
	else if(CurrentFocusItem)  //物品离开注视
	{
		CurrentFocusItem->OnItemLeaveFocused.Broadcast(MyController);
		CurrentFocusItem = nullptr;
	}
}


void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	DOREPLIFETIME(UInventoryComponent, InventorySize);
	DOREPLIFETIME(UInventoryComponent, ItemList);
	DOREPLIFETIME(UInventoryComponent, ThrownItem);
	DOREPLIFETIME(UInventoryComponent, LastChangedItemSlot);
}
