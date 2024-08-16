// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerController.h"

#include "TPSGameMode.h"
#include "GameFramework/PlayerState.h"
#include "HUD/TPSHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "UserWidget/ReturnToMainMenu.h"

ATPSPlayerController::ATPSPlayerController()
{
	//背包组件初始化
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
}

void ATPSPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckTimeSync(DeltaSeconds);
	
	if(IsLocalController())
	{
		CheckLag();
	}
}

void ATPSPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if(IsLocalController())  //若在服务器调用，服务器有多个Controller，需找到自己的那个
	{
		ServerRequestServerTime(GetWorld()->GetDeltaSeconds());
	}
}

void ATPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if(InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ATPSPlayerController::ShowReturnToMainMenu);
}

void ATPSPlayerController::ShowReturnToMainMenu()
{
	if(ReturnToMainMenuClass == nullptr) return;
	
	if(ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuClass);
	}
	if(ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if(bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
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

void ATPSPlayerController::SetCrossHairVisibility(bool IsVisible)
{
	ATPSHUD* CurHUD = Cast<ATPSHUD>(GetHUD());
	if(CurHUD)
	{
		CurHUD->SetCrossHairVisibility(IsVisible);
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

void ATPSPlayerController::CheckLag()  //在Tick()中每帧执行
{
	if(IsLocalController() && PlayerState)
	{
		//Ping是uint8类型数据且被UE除以了4，所以乘以4才是真实延迟
		if(PlayerState->GetPing() * 4 >= HighPingThreshold)
		{
			if(!GetWorldTimerManager().IsTimerActive(FShowLagIconHandle))
			{
				GetWorldTimerManager().SetTimer(
					FShowLagIconHandle,
					[this]()->void
					{
						OnLagDetected.Broadcast();
					},
					HighPingLoopDuration,
					true);
			}
		}
		else
		{
			if(GetWorldTimerManager().IsTimerActive(FShowLagIconHandle))
			{
				GetWorldTimerManager().ClearTimer(FShowLagIconHandle);
				OnLagEnded.Broadcast();
			}
		}
	}
}

void ATPSPlayerController::AskForLagSituation()
{
	if(IsLocalController() && PlayerState && PlayerState->GetPing() * 4 >= HighPingThreshold)
	{
		GetWorldTimerManager().ClearTimer(FShowLagIconHandle);
		GetWorldTimerManager().SetTimer(
			FShowLagIconHandle,
			[this]()->void
			{
				OnLagDetected.Broadcast();
			},
			HighPingLoopDuration,
			true,
			0);
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