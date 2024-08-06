// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerController.h"

#include "TPSGameMode.h"
#include "HUD/TPSHUD.h"
#include "Kismet/GameplayStatics.h"

ATPSPlayerController::ATPSPlayerController()
{
	//背包组件初始化
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void ATPSPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckTimeSync(DeltaSeconds);
}

void ATPSPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if(IsLocalController())  //若在服务器调用，服务器有多个Controller，需找到自己的那个
	{
		ServerRequestServerTime(GetWorld()->GetDeltaSeconds());
	}
}

void ATPSPlayerController::RequestRespawn_Implementation()
{
	ATPSGameMode* GM = Cast<ATPSGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if(GM)
	{
		GM->RespawnPlayer(this);
	}
}

void ATPSPlayerController::ResetCrossHair()
{
	ATPSHUD* CurHUD = Cast<ATPSHUD>(GetHUD());
	if(CurHUD)
	{
		CurHUD->ResetCrossHairWidget(this);
	}
}

void ATPSPlayerController::RemoveCrossHair()
{
	ATPSHUD* CurHUD = Cast<ATPSHUD>(GetHUD());
	if(CurHUD)
	{
		CurHUD->RemoveCrossHairWidget();
	}
}

float ATPSPlayerController::GetServerTime()
{
	if(HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}

void ATPSPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if(IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ATPSPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ATPSPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	//服务器传回的时间+服务器返回消息的延迟等于服务器当前时间，这里粗略地估算返回延迟为客户端rpc+服务器rpc延迟总和的一半
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	const float CurrentServerTime = TimeServerReceivedClientRequest + RoundTripTime/2;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}