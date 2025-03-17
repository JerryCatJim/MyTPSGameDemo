// Fill out your copyright notice in the Description page of Project Settings.


#include "SPowerUpActor.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPowerUpActor::ASPowerUpActor()
{
	PowerUpInterval = 0.0f;
	MaxOverlyingNum = 1;
	SingleOverlyingNum = 1;

	bCanDecay = false;
	bRefreshImmediately = true;
	
	CurrentOverlyingNum = 0;

	PowerUpID = 0;
	
	SetReplicates(true);  //默认打开网络复制
	bIsActive = false;
}

// Called when the game starts or when spawned
void ASPowerUpActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASPowerUpActor::OnActivated(int CurrentBuffLayer)
{
	//方便扩展
	//

	//将拾取了该道具的Actor传给蓝图
	OnActivated_BP(CurrentOverlapActor, CurrentBuffLayer);
}

void ASPowerUpActor::OnExpired()
{
	CurrentOverlapActor = nullptr;
	
	bIsActive = false;
	//同ActivePowerUp()，仅当主控权为服务器时 才会调用到这里
	if(HasAuthority())
	{
		//如上方注释，仅当服务器主控才会走到这里，加不加if判断都行
		OnRep_PowerUpActive();  //重点：C++代码内  服务器端bIsActive发生改变不会调用OnRep，需手动调用（蓝图中不需要手动调用）
	}
	
	OnExpired_BP();

	if(GetWorld())
	{
		//关闭计时器
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerUpTick);
	}
	
	Destroy();
}

void ASPowerUpActor::OnPowerUpTicked() //处理Buff持续时间结束时的函数
{
	if(!bCanDecay || PowerUpInterval <= 0.0f)  //间隔小于等于0时根本没有计时器，也走这里，直接失效
	{
		CurrentOverlyingNum = 0;
		
		OnExpired();
	}
	else  //层层衰减，而不是瞬间归零
	{
		--CurrentOverlyingNum;
		if(CurrentOverlyingNum <=0 )
		{
			CurrentOverlyingNum = 0;  //防止奇怪Bug变为负数(?)
			
			OnExpired();
			return;
		}
		OnActivated(CurrentOverlyingNum);
	}
}

void ASPowerUpActor::RefreshBuffState()
{
	if(!bRefreshImmediately)
	{
		return;
	}

	if(GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerUpTick);
		GetWorldTimerManager().SetTimer(TimerHandle_PowerUpTick, this, &ASPowerUpActor::OnPowerUpTicked, PowerUpInterval, true, -1);
	}
	OnActivated(CurrentOverlyingNum);
}


//道具被拾取后 开始生效
void ASPowerUpActor::ActivePowerUp(AActor* OverlapActor)  //详见SPickUpActor.cpp中重叠事件里，仅当主控权为服务器时 才会调用到这里
{
	CurrentOverlapActor = OverlapActor;  //拾取了该道具的Actor（Character）
	
	bIsActive = true;    //客户端上该值通过网络复制发生改变会自动调用OnRep_PowerUpActive();
	if(HasAuthority())
	{
		//如上方注释，仅当服务器主控才会走到这里，加不加if判断都行
		OnRep_PowerUpActive();  //重点：C++代码内  服务器端bIsActive发生改变不会调用OnRep，需手动调用（蓝图中不需要手动调用）
	}
	
	const int TempCurNum = CurrentOverlyingNum;  //储存拾取前的当前层数
	
	CurrentOverlyingNum = FMath::Min(CurrentOverlyingNum+SingleOverlyingNum, MaxOverlyingNum);
	
	//首次获取该效果需立刻生效
	if(TempCurNum <= 0)
	{
		//传入层数 为了自定义每层Buff效果不同
		OnActivated(CurrentOverlyingNum);  //道具被拾取 生效后的逻辑,在蓝图中实现
		
		if(PowerUpInterval > 0.0f)
		{
			//FirstDelay大于等于0时延迟此值，否则延迟InRate，不填默认-1
			GetWorldTimerManager().SetTimer(TimerHandle_PowerUpTick, this, &ASPowerUpActor::OnPowerUpTicked, PowerUpInterval, true, -1);
		}
		else
		{
			OnPowerUpTicked();
		}
	}
	else //层数没掉光时又获取道具效果则判断是否需要立刻刷新
	{
		RefreshBuffState();
	}
	
}

int ASPowerUpActor::GetPowerUpID()
{
	return PowerUpID;
}

int ASPowerUpActor::GetCurrentOverlyingNum()
{
	return CurrentOverlyingNum;
}

void ASPowerUpActor::OnRep_PowerUpActive()
{
	//蓝图中实现的函数
	OnActiveStateChanged_BP();
}

//一个模板
void ASPowerUpActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//指定网络复制哪一部分（一个变量）
	DOREPLIFETIME(ASPowerUpActor, bIsActive);
}

