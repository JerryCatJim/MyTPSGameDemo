// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/WeaponManagerComponent.h"
#include "Weapon/PickUpWeapon.h"
#include "TPSPlayerController.h"
#include "SCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UWeaponManagerComponent::UWeaponManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);

	CurrentWeaponSocketName   = "CurrentWeaponSocket";
	MainWeaponSocketName      = "MainWeaponSocket";
	SecondaryWeaponSocketName = "SecondaryWeaponSocket";
	MeleeWeaponSocketName     = "MeleeWeaponSocket";
	ThrowableWeaponSocketName = "ThrowableWeaponSocket";

	bIsAiming = false;
	bIsFiring = false;
}

// Called when the game starts
void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	MyOwnerPlayer = Cast<ASCharacter>(GetOwner());
	
	//如果控制权在服务器Server(相对Client)则执行下列代码
	if(MyOwnerPlayer && MyOwnerPlayer->HasAuthority())
	{
		//生成默认武器并吸附到角色部位
		for(int i = 0; i < WeaponEquipTypeList.Num(); ++i)
		{
			FWeaponPickUpInfo TempInfo = FWeaponPickUpInfo();
			TempInfo.WeaponClass = GetWeaponSpawnClass(WeaponEquipTypeList[i]);
			
			//武器当前还为空值，取不到自身PickUpWeaponInfo，所以手动造一个，并且不刷新信息(这样造出来的武器的WeaponPickUpInfo的是默认值)
			ASWeapon*& CurWeapon = GetWeaponByEquipType(WeaponEquipTypeList[i]) = SpawnAndAttachWeapon(TempInfo, false);
			
			if(!CurrentWeapon && CurWeapon)  //把有效的第一把武器设置为当前默认武器
			{
				CurrentWeapon = CurWeapon;
				CurrentWeapon->AttachToComponent(MyOwnerPlayer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponSocketName);
				//C++中的服务器不会自动调用OnRep函数，需要手动调用
				OnRep_CurrentWeapon();
			}
		}
	}
}


// Called every frame
void UWeaponManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

ASWeapon* UWeaponManagerComponent::SpawnAndAttachWeapon(FWeaponPickUpInfo WeaponInfo, bool RefreshWeaponInfo)
{
	if(!MyOwnerPlayer) return nullptr;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = MyOwnerPlayer;  //要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空

	//如果Spawn的Class不存在则不会生成
	ASWeapon* NewWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponInfo.WeaponClass, FVector().ZeroVector, FRotator().ZeroRotator, SpawnParams);
	//NewWeapon->SetOwner(this);
	if(NewWeapon && MyOwnerPlayer)
	{
		//如果角色身上有对应插槽则吸附，否则说明忘了配置了或者写错名字了，武器就掉到地上
		if(MyOwnerPlayer->GetMesh()->GetSocketByName(GetWeaponSocketName(NewWeapon->GetWeaponEquipType())))
		{
			NewWeapon->AttachToComponent(MyOwnerPlayer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetWeaponSocketName(NewWeapon->GetWeaponEquipType()));
		}
		else
		{
			NewWeapon->SetActorLocation(MyOwnerPlayer->GetActorLocation());
		}
		if(RefreshWeaponInfo)
		{
			//更新武器信息
			NewWeapon->RefreshWeaponInfo(WeaponInfo);
		}
	}
	return NewWeapon;
}

TSubclassOf<ASWeapon> UWeaponManagerComponent::GetWeaponSpawnClass(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	TSubclassOf<ASWeapon> WSClass = MainWeaponClass; 
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		WSClass = MainWeaponClass;
		break;
	case EWeaponEquipType::SecondaryWeapon:
		WSClass = SecondaryWeaponClass;
		break;
	case EWeaponEquipType::MeleeWeapon:
		WSClass = MeleeWeaponClass;
		break;
	case EWeaponEquipType::ThrowableWeapon:
		WSClass = ThrowableWeaponClass;
		break;
	default:
		break;
	}
	return WSClass;
}

FName UWeaponManagerComponent::GetWeaponSocketName(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	FName WSName = MainWeaponSocketName; 
	switch(WeaponEquipType)
	{
		case EWeaponEquipType::MainWeapon:
			WSName = MainWeaponSocketName;
			break;
		case EWeaponEquipType::SecondaryWeapon:
			WSName = SecondaryWeaponSocketName;
			break;
		case EWeaponEquipType::MeleeWeapon:
			WSName = MeleeWeaponSocketName;
			break;
		case EWeaponEquipType::ThrowableWeapon:
			WSName = ThrowableWeaponSocketName;
			break;
		default:
			break;
	}
	return WSName;
}

ASWeapon*& UWeaponManagerComponent::GetWeaponByEquipType(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		return MainWeapon;
	case EWeaponEquipType::SecondaryWeapon:
		return SecondaryWeapon;
	case EWeaponEquipType::MeleeWeapon:
		return MeleeWeapon;
	case EWeaponEquipType::ThrowableWeapon:
		return ThrowableWeapon;
	default:
		return MainWeapon;
	}
}

UAnimMontage* UWeaponManagerComponent::GetSwapWeaponAnim(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		return SwapMainWeaponAnim;
	case EWeaponEquipType::SecondaryWeapon:
		return SwapSecondaryWeaponAnim;
	case EWeaponEquipType::MeleeWeapon:
		return SwapMeleeWeaponAnim;
	case EWeaponEquipType::ThrowableWeapon:
		return SwapThrowableWeaponAnim;
	default:
		return nullptr;
	}
}

float UWeaponManagerComponent::GetSwapWeaponAnimRate(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	float SwapRate = 1.f;
	switch(WeaponEquipType)
	{
	case EWeaponEquipType::MainWeapon:
		SwapRate = SwapMainWeaponRate;
		break;
	case EWeaponEquipType::SecondaryWeapon:
		SwapRate = SwapSecondaryWeaponRate;
		break;
	case EWeaponEquipType::MeleeWeapon:
		SwapRate = SwapMeleeWeaponRate;
		break;
	case EWeaponEquipType::ThrowableWeapon:
		SwapRate = SwapThrowableWeaponRate;
		break;
	default:
		break;
	}
	return SwapRate;
}

void UWeaponManagerComponent::SwapToNextAvailableWeapon(TEnumAsByte<EWeaponEquipType> CurrentWeaponEquipType)
{
	const TEnumAsByte<EWeaponEquipType> CurWeaponType = IsValid(CurrentWeapon) ? CurrentWeapon->GetWeaponEquipType() : CurrentWeaponEquipType;
	const int StartPos = WeaponEquipTypeList.IndexOfByKey(CurWeaponType);
	for(int i = StartPos; i < 2; ++i) //暂时只计算 主武器 副武器 这两种,如果已经是刀/拳头则无事发生，后续可扩充修改为循环切换
	{
		for(int j = i + 1; j < 3; ++j)
		{
			if(IsValid(GetWeaponByEquipType(WeaponEquipTypeList[j])))
			{
				StartSwapWeapon(WeaponEquipTypeList[j],true);
				return;
			}
		}
	}
}

void UWeaponManagerComponent::DealSwapWeaponAttachment(TEnumAsByte<EWeaponEquipType> WeaponEquipType)
{
	if(!MyOwnerPlayer) return;
	
	if(!IsValid(GetWeaponByEquipType(WeaponEquipType)))
	{
		return;
	}
	
	if(IsValid(CurrentWeapon))
	{
		//若要切换位置的武器已经被装备为当前武器则无事发生
		if(CurrentWeapon == GetWeaponByEquipType(WeaponEquipType))
		{
			StopSwapWeapon(false);
			return;
		}
		CurrentWeapon->AttachToComponent(MyOwnerPlayer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetWeaponSocketName(CurrentWeapon->GetWeaponEquipType()));
	}
	//可以先将武器交换吸附这种纯表现的逻辑在双端都进行，但是实际更改CurrentWeapon赋值的行为只在服务器进行
	GetWeaponByEquipType(WeaponEquipType)->AttachToComponent(MyOwnerPlayer->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentWeaponSocketName);
	
	if(!MyOwnerPlayer->HasAuthority())
	{
		//客户端需等待服务器完成判断，在此之前本地虽结束了动画播放和计时器，但bIsSwapping状态应该仍为true，以禁止当前武器开火
		StopSwapWeapon(true);
		return;
	}

	if(MyOwnerPlayer->HasAuthority())
	{
		CurrentWeapon = GetWeaponByEquipType(WeaponEquipType);
		//OnRep中会调用StopSwapWeapon(false)将bIsSwappingWeapon变为false
		OnRep_CurrentWeapon();
	}
}

//将开镜行为发送到服务器然后同步
void UWeaponManagerComponent::SetWeaponZoom()
{
	if(!MyOwnerPlayer) return;
	
	//空手不能开镜
	if(CurrentWeapon)
	{
		CurrentWeapon->SetWeaponZoom();
		if(IsLocallyControlled())
		{
			bIsAiming = true;
			bIsAimingLocally = true;
		}
	}
}

void UWeaponManagerComponent::ResetWeaponZoom()
{
	if(!MyOwnerPlayer) return;
	
	if(CurrentWeapon)
	{
		CurrentWeapon->ResetWeaponZoom();
		if(IsLocallyControlled())
		{
			bIsAiming = false;
			bIsAimingLocally = false;
		}
	}
}

void UWeaponManagerComponent::PickUpWeapon(FWeaponPickUpInfo WeaponInfo)
{
	if(!MyOwnerPlayer) return;
	
	//要先StopFire再StopReload，否则会导致0子弹时更换武器，StopFire中因为0子弹又执行了一次Reload，生成武器后的新武器子弹数没及时刷新时又执行一次换弹
	//(当然，你也可以生成时再执行一次StopReload，但我感觉那样不好)
	StopFire();
	StopReload();
	StopSwapWeapon(false);
	
	//客户端或服务端都应在拾取武器时停止开火，但是生成新武器等操作仅在服务端执行
	DealPickUpWeapon(WeaponInfo);
}

void UWeaponManagerComponent::DealPickUpWeapon_Implementation(FWeaponPickUpInfo WeaponInfo)
{
	ASWeapon*& WeaponToExchange = GetWeaponByEquipType(WeaponInfo.WeaponEquipType);
	FWeaponPickUpInfo InfoToBroadcast = IsValid(WeaponToExchange) ? WeaponToExchange->GetWeaponPickUpInfo() : FWeaponPickUpInfo();
	//把旧武器信息广播出去，可用于和地上可拾取武器的信息互换,广播出去空信息被PickUpWeapon接收到后，会导致PickUpWeapon被销毁
	OnExchangeWeapon.Broadcast(InfoToBroadcast);
	
	if(!WeaponToExchange)  //如果没装备对应位置的武器则直接拾取
	{
		WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
		if(!WeaponToExchange)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5 ,FColor::Red, TEXT("生成待拾取的武器失败!"));
			return;
		}
		if(!IsValid(CurrentWeapon) || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::MeleeWeapon || CurrentWeapon->GetWeaponEquipType() == EWeaponEquipType::ThrowableWeapon)
		{
			StartSwapWeapon(WeaponInfo.WeaponEquipType, true);
		}
	}
	else
	{
		WeaponToExchange->Destroy();
		WeaponToExchange = SpawnAndAttachWeapon(WeaponInfo);
		if(!WeaponToExchange)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5 ,FColor::Red, TEXT("生成待拾取的武器失败!"));
			return;
		}
		if(!IsValid(CurrentWeapon)) //如果当前武器类型和要拾取的类型相同，则销毁后CurrentWeapon会无效(不为nullptr)，此时执行交换逻辑正好
		{
			StartSwapWeapon(WeaponToExchange->GetWeaponEquipType(), true);
		}
	}
}

void UWeaponManagerComponent::StartDropWeapon(bool ManuallyDiscard)
{
	if(!MyOwnerPlayer) return;
	
	if(!MyOwnerPlayer->HasAuthority())
	{
		//客户端提前预测，以停止当前行动等待服务器同步，提升客户端在高延迟下的观感，不保持也不影响最终的数据同步
		if(!IsValid(CurrentWeapon) || (!ManuallyDiscard && !CurrentWeapon->GetWeaponCanDropDown())
		|| (ManuallyDiscard && !CurrentWeapon->GetWeaponCanManuallyDiscard()))
		{
			return;
		}
		StopFire();
		StopReload();
		StopSwapWeapon(false);
	}
	DealDropWeapon(ManuallyDiscard);  //Server方法
}

void UWeaponManagerComponent::DealDropWeapon_Implementation(bool ManuallyDiscard)
{
	if(!MyOwnerPlayer) return;
	
	if(!IsValid(CurrentWeapon) || (!ManuallyDiscard && !CurrentWeapon->GetWeaponCanDropDown())
		|| (ManuallyDiscard && !CurrentWeapon->GetWeaponCanManuallyDiscard()))
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if(World)
	{
		TSubclassOf<APickUpWeapon> PickUpWeaponClass = APickUpWeapon::StaticClass();
		if(IsValid(CurrentWeapon) && CurrentWeapon->GetPickUpWeaponClass())
		{
			PickUpWeaponClass = CurrentWeapon->GetPickUpWeaponClass();
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
				FString::Printf(TEXT("CurrentWeapon的PickUpWeaponClass不存在！会导致生成的可拾取武器不会刷新信息！")));
		}

		//PickUpWeaponClass如果没有指定默认的SkeletalMesh(即默认为空指针)时,
		//会导致其实例在BeginPlay中设置完Mesh后立刻SetSimulatePhysics(true)时触发断言而游戏崩溃,
		//因为复制时Mesh还不存在而触发断言checkSlow(RootComponent->IsSimulatingPhysics())为false.
		APickUpWeapon* PickUpWeapon = World->SpawnActorDeferred<APickUpWeapon>(
			PickUpWeaponClass,
			FTransform(MyOwnerPlayer->GetActorLocation() + MyOwnerPlayer->GetActorForwardVector() *50 )
			);
		if(PickUpWeapon)
		{
			PickUpWeapon->SetOwner(MyOwnerPlayer);
			PickUpWeapon->DefaultEditWeaponClass = nullptr;
			PickUpWeapon->bThrowAfterSpawn = ManuallyDiscard;
			
			PickUpWeapon->WeaponPickUpInfo = FWeaponPickUpInfo(
				MyOwnerPlayer,
				CurrentWeapon->GetWeaponMeshComp()->SkeletalMesh,
				CurrentWeapon->GetClass(),
				CurrentWeapon->CurrentAmmoNum,
				CurrentWeapon->BackUpAmmoNum,
				CurrentWeapon->WeaponName,
				CurrentWeapon->GetWeaponEquipType(),
				true
				);
			//从C++中获取蓝图类
			const FString WidgetClassLoadPath = FString(TEXT("/Game/UI/WBP_ItemPickUpTip.WBP_ItemPickUpTip_C"));//蓝图一定要加_C这个后缀名
			UClass* Widget = LoadClass<UUserWidget>(nullptr, *WidgetClassLoadPath);
			PickUpWeapon->WidgetComponent->SetWidgetClass(Widget);
			PickUpWeapon->bCanMeshDropOnTheGround = true;

			//主动扔掉的武器不能长按刷新为原武器(场景内摆放的或者什么道具点刷新的才可以)
			PickUpWeapon->bCanInteractKeyLongPress = false;

			//完成SpawnActor
			PickUpWeapon->FinishSpawning(FTransform(MyOwnerPlayer->GetActorLocation() + MyOwnerPlayer->GetActorForwardVector() *50));

			StopFire();
			StopReload();
			StopSwapWeapon(false);
			//掉落武器后销毁当前武器并自动切换为下一把
			const EWeaponEquipType CurType = CurrentWeapon->GetWeaponEquipType();
			CurrentWeapon->Destroy();
			GetWeaponByEquipType(CurType) = nullptr;
			SwapToNextAvailableWeapon(CurType);
		}
	}
}


void UWeaponManagerComponent::StartFire()
{
	if(!MyOwnerPlayer) return;
	
	if(CurrentWeapon)
	{
		//有子弹或者无限子弹才设置为开火状态，防止一按开火人物就瞬间面对正前方
		bool PreCheckRes = CurrentWeapon->CheckCanFire();
		CurrentWeapon->StartFire();
		if(IsLocallyControlled())
		{
			bIsFiring        = PreCheckRes;
			bIsFiringLocally = PreCheckRes;
		}
	}
}

void UWeaponManagerComponent::StopFire()
{
	if(!MyOwnerPlayer) return;
	
	if(CurrentWeapon && !GetIsSwappingWeapon())
	{
		CurrentWeapon->StopFire();
		if(IsLocallyControlled())
		{
			bIsFiring = false;
			bIsFiringLocally = false;
		}
	}
}

void UWeaponManagerComponent::StartReload()
{
	if(!MyOwnerPlayer) return;
	
	if(CurrentWeapon)
	{
		bool PreCheckRes = CurrentWeapon->CheckCanReload();
		CurrentWeapon->StartReload();
		if(IsLocallyControlled())
		{
			//正在换弹时再按换弹不会打断换单操作，且仍为换弹状态
			bIsReloading        = PreCheckRes || GetIsReloading();
			bIsReloadingLocally = PreCheckRes || GetIsReloading();
		}
	}
}

void UWeaponManagerComponent::StopReload()
{
	if(!MyOwnerPlayer) return;
	
	if(CurrentWeapon)
	{
		CurrentWeapon->StopReload();
		if(IsLocallyControlled())
		{
			bIsReloading = false;
			bIsReloadingLocally = false;
		}
	}
}

void UWeaponManagerComponent::StartSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(!MyOwnerPlayer || !IsValid(GetWeaponByEquipType(NewWeaponEquipType)) || (CurrentWeapon == GetWeaponByEquipType(NewWeaponEquipType) && !GetIsSwappingWeapon()))
	{
		return;
	}
	
	//如果刚切枪到一半(此时还未完成交换)时又按了一下想切回原武器则允许交换，最终进入DealPlaySwapWeaponAnim的反播动画部分
	SwapWeapon(NewWeaponEquipType, Immediately);
	if(IsLocallyControlled())
	{
		bIsSwappingWeapon = !Immediately;
		bIsSwappingWeaponLocally = !Immediately;
	}
}

void UWeaponManagerComponent::StopSwapWeapon(bool bWaitCurrentWeaponReplicated)
{
	if(!MyOwnerPlayer) return;
	
	MyOwnerPlayer->GetWorldTimerManager().ClearTimer(SwapWeaponTimer);
	MyOwnerPlayer->StopAnimMontage(CurrentSwapWeaponAnim);
	CurrentSwapWeaponAnim = nullptr;

	if(MyOwnerPlayer->HasAuthority())
	{
		bIsSwappingWeapon = false;
		bIsSwappingWeaponLocally = false;
		return;
	}
	
	if(IsLocallyControlled())
	{
		//客户端需等待武器真正完成复制后才算完成交换(未实际交换时bWaitCurrentWeaponReplicated会填入false)
		bIsSwappingWeapon = bWaitCurrentWeaponReplicated;
		bIsSwappingWeaponLocally = bWaitCurrentWeaponReplicated;
	}
}

//外部操作不要直接调用这个函数，调用StartSwapWeapon()
void UWeaponManagerComponent::SwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(!MyOwnerPlayer) return;
	
	//如果刚切枪到一半(此时还未完成交换)时又按了一下想切回原武器则允许交换，最终进入DealPlaySwapWeaponAnim的反播动画部分
	if(IsValid(CurrentWeapon) && CurrentWeapon == GetWeaponByEquipType(NewWeaponEquipType) && !GetIsSwappingWeapon())
	{
		return;
	}
	
	StopFire();
	StopReload();
	
	if(!MyOwnerPlayer->HasAuthority())  //本地只处理一些表现效果，权威端在服务器
	{
		//分开处理是为了高延迟下客户端也能立刻响应一些表现
		ServerSwapWeapon(NewWeaponEquipType, Immediately);
		LocalSwapWeapon(NewWeaponEquipType, Immediately);
		return;
	}
	
	bIsSwappingWeapon = true;
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
	//因为双端分别执行Swap没用Multi，所以如果是服务器主控玩家，则需要同步播放Swap动画等表现到客户端
	if(MyOwnerPlayer->HasAuthority() && IsLocallyControlled())
	{
		Multi_ClientSyncPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
	}
}

void UWeaponManagerComponent::LocalSwapWeapon(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	//本地会预测结果，所以把bIsSwappingWeapon = true;从DealPlaySwapWeaponAnim中剥离到Server
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
}

void UWeaponManagerComponent::ServerSwapWeapon_Implementation(EWeaponEquipType NewWeaponEquipType, bool Immediately)
{
	SwapWeapon(NewWeaponEquipType, Immediately);
}

void UWeaponManagerComponent::DealPlaySwapWeaponAnim(TEnumAsByte<EWeaponEquipType> NewWeaponEquipType, bool Immediately)
{
	if(Immediately)
	{
		if(MyOwnerPlayer->HasAuthority())  //客户端只播放动画
		{
			DealSwapWeaponAttachment(NewWeaponEquipType);
		}
	}
	else
	{
		UAnimMontage* SwapAnim = GetSwapWeaponAnim(NewWeaponEquipType);
		float SwapAnimPlayRate = GetSwapWeaponAnimRate(NewWeaponEquipType);
		float MontagePlayTime = SwapAnim && SwapAnimPlayRate>0.f ? SwapAnim->SequenceLength/SwapAnimPlayRate : 0.f ;
		
		//如果上次切换动画播到一半又想切回原武器，只需反播当前动画
		if(GetIsSwappingWeapon() && CurrentSwapWeaponAnim && NewWeaponEquipType == CurrentWeapon->GetWeaponEquipType())
		{
			SwapAnim = CurrentSwapWeaponAnim;
			
			const UAnimInstance* AnimIns = MyOwnerPlayer->GetMesh()->GetAnimInstance();
			MontagePlayTime = AnimIns->Montage_GetPosition(SwapAnim)/SwapAnimPlayRate;
			//播放速率变为复数即为反着播放(?)
			SwapAnimPlayRate = SwapAnimPlayRate * -1.f;
		}
		if(SwapAnim)
		{
			if(MontagePlayTime > 0)
			{
				CurrentSwapWeaponAnim = SwapAnim;
				
				MyOwnerPlayer->PlayAnimMontage(CurrentSwapWeaponAnim, SwapAnimPlayRate);

				FTimerDelegate STD;  //SwapWeaponTimerDelegate
				//注意Lambda表达式的捕获方式如果为&则可能导致当前作用域内变量到生命周期之后再执行Timer时获取不到值而无法执行Lambda方法
				STD.BindWeakLambda(this,[=]
				{
					//无论正反播放动画都可以走进交换武器逻辑
					//只不过反向播放时CurrentWeapon还没有完成替换，会卡在DealSwapWeaponAttachment的判断那步，自然就不会完成切换
					DealSwapWeaponAttachment(NewWeaponEquipType);
				});
				
				float TimerRate = SwapAnimPlayRate > 0 ? 0.8 : 0.7;//反播动画时候稍微快点结束
				//GetWorldTimerManager().ClearTimer(SwapWeaponTimer);  //Set已存在的Timer会自动先Clear
				MyOwnerPlayer->GetWorldTimerManager().SetTimer(
					SwapWeaponTimer,
					STD,
					MontagePlayTime* TimerRate,
					false);
			}
			else
			{
				const UEnum* TestEnumPtr = StaticEnum<EWeaponEquipType>();
				GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
					FString::Printf(TEXT("%s的 SwapAnimPlayRate < 0 ！"),*TestEnumPtr->GetDisplayNameTextByValue(static_cast<uint8>(NewWeaponEquipType)).ToString()));
			
				if(MyOwnerPlayer->HasAuthority())  //客户端只播放动画
				{
					DealSwapWeaponAttachment(NewWeaponEquipType);
				}
			}
		}
		else
		{
			const UEnum* TestEnumPtr = StaticEnum<EWeaponEquipType>();
			GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red,
				FString::Printf(TEXT("Swap%sAnim蒙太奇不存在！"),*TestEnumPtr->GetDisplayNameTextByValue(static_cast<uint8>(NewWeaponEquipType)).ToString()));
				
			if(MyOwnerPlayer->HasAuthority())  //客户端只播放动画
			{
				DealSwapWeaponAttachment(NewWeaponEquipType);
			}
		}
	}
}

void UWeaponManagerComponent::Multi_ClientSyncPlaySwapWeaponAnim_Implementation(EWeaponEquipType NewWeaponEquipType, bool Immediately)
{
	if(!MyOwnerPlayer || MyOwnerPlayer->HasAuthority() && IsLocallyControlled())
	{
		return;
	}
	DealPlaySwapWeaponAnim(NewWeaponEquipType, Immediately);
}

void UWeaponManagerComponent::OnRep_CurrentWeapon()
{
	if(!MyOwnerPlayer) return;
	
	ATPSPlayerController* MyController = Cast<ATPSPlayerController>(MyOwnerPlayer->GetController());
	if(MyController)  //服务端RestartPlayer时先触发BeginPlay然后OnPossess，BeginPlay时Controller暂时为空，所以在PossessedBy中再触发一次设置准星
	{
		MyController->ResetCrossHair();
	}
	ShowAutoLockEnemyTipView();
	
	//服务器调用OnRep时会立刻将bIsSwappingWeapon赋值为false，客户端需等待OnRep完成复制时才改为false以保持同步
	StopSwapWeapon(false);
	
	OnCurrentWeaponChanged.Broadcast();
}

void UWeaponManagerComponent::ShowAutoLockEnemyTipView()
{
	if(!CurrentWeapon || !IsLocallyControlled()) return;
	
	bool IsAutoLock = CurrentWeapon->GetIsAutoLockEnemy();
	if(IsAutoLock)
	{
		if(AutoLockEnemyTipView)
		{
			AutoLockEnemyTipView->RemoveFromParent();
			AutoLockEnemyTipView = nullptr;
		}
		if(!CurrentWeapon->AutoLockEnemyTipClass)
		{
			//从C++中获取蓝图类
			const FString AutoLockEnemyTipClassLoadPath = FString(TEXT("/Game/UI/WBP_AutoLockTip.WBP_AutoLockTip_C"));//蓝图一定要加_C这个后缀名
			CurrentWeapon->AutoLockEnemyTipClass = LoadClass<UUserWidget>(nullptr, *AutoLockEnemyTipClassLoadPath);
		}
		if(CurrentWeapon->AutoLockEnemyTipClass && CurrentWeapon->GetOwner())
		{
			APawn* TempOwner = Cast<APawn>(CurrentWeapon->GetOwner());
			if(TempOwner)
			{
				APlayerController* PC = Cast<APlayerController>(TempOwner->GetController());
				//APlayerController* PC = UGameplayStatics::GetPlayerController(this,0);
				AutoLockEnemyTipView = CreateWidget<UUserWidget>(PC, CurrentWeapon->AutoLockEnemyTipClass);
				if(AutoLockEnemyTipView)
				{
					AutoLockEnemyTipView->AddToViewport();
				}
			}
		}
	}
	else
	{
		if(AutoLockEnemyTipView)
		{
			AutoLockEnemyTipView->RemoveFromParent();
			AutoLockEnemyTipView = nullptr;
		}
	}
}

bool UWeaponManagerComponent::IsLocallyControlled()
{
	return MyOwnerPlayer && MyOwnerPlayer->IsLocallyControlled();
}

void UWeaponManagerComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if(AutoLockEnemyTipView)
	{
		AutoLockEnemyTipView->RemoveFromParent();
		AutoLockEnemyTipView = nullptr;
	}
}

void UWeaponManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponManagerComponent, CurrentWeapon);
	
	DOREPLIFETIME(UWeaponManagerComponent, MainWeapon);
	DOREPLIFETIME(UWeaponManagerComponent, SecondaryWeapon);
	DOREPLIFETIME(UWeaponManagerComponent, MeleeWeapon);
	DOREPLIFETIME(UWeaponManagerComponent, ThrowableWeapon);
	
	DOREPLIFETIME(UWeaponManagerComponent, bIsAiming);
	DOREPLIFETIME(UWeaponManagerComponent, bIsFiring);
	DOREPLIFETIME(UWeaponManagerComponent, bIsReloading);
	DOREPLIFETIME(UWeaponManagerComponent, bIsSwappingWeapon);
}
