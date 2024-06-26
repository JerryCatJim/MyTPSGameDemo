// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FMyItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)  //记录物品Class，以便在丢弃生成到世界时可以获取其Class
	TSubclassOf<class AInventoryItemBase> ItemClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemName = "EmptyItem";

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ItemID = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ItemQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanStack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int ItemIndex = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemListChanged);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class TPSGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitInventoryComponent();

	//Controller生成时还没有ControlledPawn，需要用到时尝试获取
	void TrySetOwnerPawn();
	
	int GetCurrentSize() const;
	
	int FindEmptyPos() const;

	UFUNCTION(BlueprintCallable)
	int FindFirstFilledPos() const;

	bool CheckIsValidIndex(int Index) const;
	//留给外部调用的接口
	UFUNCTION(BlueprintCallable)
	void InputAddItem();
	UFUNCTION(BlueprintCallable)
	void InputRemoveItem(int ArrayIndex = 0, int RemoveQuantity = 0);
	UFUNCTION(BlueprintCallable)
	void InputSwapItem(int FirstIndex, int SecondIndex);
	UFUNCTION(BlueprintCallable)
	void InputUseItem(int ArrayIndex = 0,  int ItemQuantity = 0);

	//添加物品
	UFUNCTION(Server, Reliable) //留给外部调用的拾取物品函数
	void AddItem(FMyItem ItemToAdd, bool IsPickUp = true);
	//在服务端处理背包拾取物品操作，然后将ItemList网络复制到客户端
	bool DealAddItem(FMyItem ItemToAdd, bool IsPickUp);
	UFUNCTION(Server, Reliable)
	void DestroyCurrentFocusItem();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)  //添加物品后的扩展操作
	bool PostAddItem(FMyItem ItemToAdd, bool IsPickUp);

	//移除物品
	UFUNCTION(Server, Reliable) //留给外部调用的移除物品函数
	void RemoveItem(int ArrayIndex = 0, int RemoveQuantity = 0, bool IsThrow = true);
	//在服务端处理背包丢弃物品操作，然后将ItemList网络复制到客户端
	bool DealRemoveItem(int ArrayIndex, int RemoveQuantity, bool IsThrow);
	UFUNCTION(Server, Reliable)
	void SpawnCurrentThrowItem(FMyItem ItemToRemove);  //在世界生成丢弃的物品
	UFUNCTION()
	void OnRep_ThrownItem();  //生成丢弃的物品后为其加一个抛出去的速度
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)  //移除物品后的扩展操作
	bool PostRemoveItem(int ArrayIndex, int RemoveQuantity);

	//互换两个格子的物品
	UFUNCTION(Server, Reliable)
	void SwapItem(int FirstIndex, int SecondIndex);
	//在服务端处理背包互换物品操作，然后将ItemList网络复制到客户端
	bool DealSwapItem(int FirstIndex, int SecondIndex);

	//使用物品
	UFUNCTION(Server, Reliable)
	void UseItem(int ArrayIndex = 0,  int ItemQuantity = 0);
	//在服务端处理背包使用物品操作，然后将ItemList网络复制到客户端
	bool DealUseItem(int ArrayIndex,  int ItemQuantity);

	UFUNCTION()  //ItemList改变时网络复制时调用
	void OnRep_ItemList();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void FocusOnPickUpItem();
public:
	const FMyItem EmptyItem;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemList)
	TArray<FMyItem> ItemList;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)  //记录了上一次被加入或丢弃物品和其ItemIndex(ItemIndex == -1 表示加入或丢弃失败)，可用于确定刷新UI的一格还是全刷新(ItemQuantity == -1 表示全刷新)
	FMyItem LastChangedItemSlot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int InventorySize;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite);
	int CurrentUsedSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TraceLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThrowForwardDistance = 100;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThrowPowerSpeed = 300;

	UPROPERTY(BlueprintAssignable)  //ItemList改变时触发广播，可用于刷新UI等
	FOnItemListChanged OnItemListChanged;
	
protected:
	UPROPERTY()
	APawn* MyOwnerPawn;  //Pawn可以被控制，设定只有Pawn以及其子类才能使用背包组件
	UPROPERTY()
	APlayerController* MyController;  //被玩家控制的角色才能使用背包组件，AIController不行
	UPROPERTY()
	class AInventoryItemBase* CurrentFocusItem = nullptr;
	UPROPERTY(ReplicatedUsing = OnRep_ThrownItem)
	class AInventoryItemBase* ThrownItem = nullptr;
	
};
