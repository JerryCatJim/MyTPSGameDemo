// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayerController.h"

#include "SCharacter.h"
#include "GameMode/TPSGameMode.h"
#include "TPSGameState.h"
#include "GameFramework/PlayerState.h"
#include "HUD/TPSHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "UserWidget/ReturnToMainMenu.h"

ATPSPlayerController::ATPSPlayerController()
{
	SetReplicates(true);
	
	//背包组件初始化
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	const FString WidgetClassLoadPath = FString(TEXT("'/Game/UI/WBP_ReturnToMainMenu.WBP_ReturnToMainMenu_C'"));//蓝图一定要加_C这个后缀名
	ReturnToMainMenuClass = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
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

void ATPSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	NewPawnClassToPossess = GetPawn()->GetClass();
	
	//PlayerController.cpp的OnPossess函数中先触发Pawn的PossessedBy再触发SetPawn()，所以走到这里时，controller还没有新的Pawn，
	//HUD里判断GetPawn()就会失败，所以要把逻辑移到Controller的OnPossess中实现
	ResetHUDWidgets(EHUDViewType::AllViews);
}

void ATPSPlayerController::RefreshScoreBoardUI_Implementation()
{
	ATPSGameState* GS = Cast<ATPSGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if(GS)
	{
		GS->RefreshPlayerScoreBoardUI(this, false);
	}
}

float ATPSPlayerController::GetRespawnCount()
{
	ATPSGameState* GS = Cast<ATPSGameState>(UGameplayStatics::GetGameState(GetWorld()));
	return GS ? GS->GetRespawnCount() : -1 ;
}

void ATPSPlayerController::SetNewPawn_Implementation(TSubclassOf<ASCharacter> NewPawnClass)
{
	if(!NewPawnClass || NewPawnClass == GetPawn()->GetClass())
	{
		NewPawnClassToPossess = GetPawn()->GetClass();
		return;
	}
	
	ASCharacter* OldCharacter = Cast<ASCharacter>(GetPawn());
	TWeakObjectPtr<ASCharacter> NewCharacter = nullptr;
	
	if(OldCharacter && !OldCharacter->GetIsDied())
	{
		FTransform OldPawnPos = OldCharacter && !OldCharacter->GetIsDied() ?
				OldCharacter->GetActorTransform()
				: FTransform(FRotator(), FVector(-10000,-10000,-10000));

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = this;  //要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空
		NewCharacter =  GetWorld()->SpawnActor<ASCharacter>(
			NewPawnClass,
			OldPawnPos,
			SpawnParams
		);

		if(NewCharacter.IsValid())
		{
			Possess(NewCharacter.Get());
			OldCharacter->Destroy(true);
		}
	}
	
	NewPawnClassToPossess = NewPawnClass;
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
		ReturnToMainMenu->ShowOrHideReturnToMainMenu();
	}
}

void ATPSPlayerController::RequestRespawn_Implementation()
{
	ATPSGameMode* GM = Cast<ATPSGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if(GM)
	{
		const float RespawnCount = GM->GetRespawnCount();
		if(RespawnCount >= 0 && GetWorld())  //复活时间小于0 约定为禁止复活
		{
			//尝试复活
			GetWorldTimerManager().SetTimer(FPlayerRespawnTimerHandle,
			[this, WeakGM = TWeakObjectPtr<ATPSGameMode>(GM)]()->void
			{
				if(WeakGM.IsValid())
				{
					WeakGM->RespawnPlayer(this);
				}
			},
			RespawnCount,
			false);
		}
		else
		{
			//设置观战之类的逻辑
		}
	}
}

void ATPSPlayerController::ResetHUDWidgets(EHUDViewType HUDViewType)
{
	switch(HUDViewType)
	{
	case EHUDViewType::CrossHairView:
		ResetCrossHair();
		break;
	case EHUDViewType::SkillPercentView:
		ResetSkillPercentWidget();
		break;
	case EHUDViewType::AllViews:
		ResetCrossHair();
		ResetSkillPercentWidget();
		break;
	default:
		break;
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

void ATPSPlayerController::ResetSkillPercentWidget()
{
	ATPSHUD* CurHUD = Cast<ATPSHUD>(GetHUD());
	if(CurHUD)
	{
		CurHUD->ResetSkillPercentWidget(this);
	}
}

void ATPSPlayerController::PlayerLeaveGame()//_Implementation()	//在ReturnToMainMenu.Cpp中被调用
{
	ASCharacter* MyCharacter = Cast<ASCharacter>(GetPawn());
	if(MyCharacter)
	{
		MyCharacter->PlayerLeaveGame();
	}

	if(GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FPlayerRespawnTimerHandle);
	}

	if(IsLocalController()) //ReturnToMainMenu是UI，存在于本地，所以限制本地发送广播
	{
		OnPlayerLeaveGame.Broadcast();  //ReturnToMainMenu.Cpp中绑定了此委托，收到后会调用OnPlayerLeftGame()销毁会话退出游戏
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
			if(GetWorld() && !GetWorldTimerManager().IsTimerActive(FShowLagIconHandle))
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
			if(GetWorld() && GetWorldTimerManager().IsTimerActive(FShowLagIconHandle))
			{
				GetWorldTimerManager().ClearTimer(FShowLagIconHandle);
				OnLagEnded.Broadcast();
			}
		}
	}
}

void ATPSPlayerController::AskForLagSituation()
{
	if(GetWorld() && IsLocalController() && PlayerState && PlayerState->GetPing() * 4 >= HighPingThreshold)
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