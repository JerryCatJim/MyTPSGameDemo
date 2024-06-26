// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Component/InventoryComponent.h"
#include "InventoryItemBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemFocused, APlayerController*, FocusingPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemLeaveFocused, APlayerController*, FocusingPC);

UCLASS()
class TPSGAME_API AInventoryItemBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInventoryItemBase();

	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	//重写父类函数,在生命周期中进行网络复制
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_TestValue();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Replicated)
	FMyItem ItemSlot;
	
	UPROPERTY(BlueprintAssignable)
	FOnItemFocused OnItemFocused;  //物品被注视时

	UPROPERTY(BlueprintAssignable)
	FOnItemLeaveFocused OnItemLeaveFocused;  //物品离开注视时

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowHighLight = true;  //被玩家注视时是否显示高亮特效

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* MeshComponent;

	//声明创建UWidgetComponent需要在build.cs文件中添加UMG模块！！！
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWidgetComponent* TextWidget;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_TestValue)
	int TestValue;
};
