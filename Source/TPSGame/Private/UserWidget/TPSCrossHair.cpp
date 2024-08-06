// Fill out your copyright notice in the Description page of Project Settings.


#include "UserWidget/TPSCrossHair.h"
#include "TimerManager.h" 
#include "SCharacter.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UTPSCrossHair::Initialize()
{
	const bool bRet = Super::Initialize();
	
	RebindMyOwner();
	
	return bRet;
}

void UTPSCrossHair::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if(bCanCrossHairSpread)
	{
		DrawCrossHairSpread();
		//GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, FString::Printf(TEXT("%f"), LastSpreadValue));
		//GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Blue, FString::Printf(TEXT("%f"), GetCrossHairPos(C_Left)));
	}
}

void UTPSCrossHair::RebindMyOwner()
{
	MyOwner = Cast<ASCharacter>(GetOwningPlayerPawn());
}

UImage* UTPSCrossHair::GetValidCrossHairLine(bool& OutIsLeftOrRight)
{
	if(C_Left)
	{
		OutIsLeftOrRight = true;
		return C_Left;
	}
	if(C_Right)
	{
		OutIsLeftOrRight = true;
		return C_Right;
	}
	if(C_Up)
	{
		OutIsLeftOrRight = false;
		return C_Up;
	}
	if(C_Down)
	{
		OutIsLeftOrRight = false;
		return C_Down;
	}
	OutIsLeftOrRight = false;
	return nullptr;
}

float UTPSCrossHair::GetCrossHairPos(UImage* CrossHair)
{
	if(!CrossHair)
	{
		return 0.f;
	}
	const FVector2D TempPosVector = UWidgetLayoutLibrary::SlotAsCanvasSlot(CrossHair)->GetPosition();
	if(CrossHair == C_Left || CrossHair == C_Right)
	{
		return FMath::Abs(TempPosVector.X);
	}
	if(CrossHair == C_Up || CrossHair == C_Down)
	{
		return FMath::Abs(TempPosVector.Y);
	}
	return 0.f;
}

void UTPSCrossHair::DrawCrossHairSpread()
{
	if(!MyOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("CrossHair的Owner不存在"));
		return;
	}

	//每帧都检测是否需要重画准星，因为准星扩散或缩小需要时间，所以设置计时器每帧循环更新准星位置
	if(LastSpreadValue != GetDynamicSpreadValue())
	{
		IsSpreading = GetDynamicSpreadValue() > LastSpreadValue;
		GetWorld()->GetTimerManager().ClearTimer(FSpreadCrossHairTimer);
		GetWorld()->GetTimerManager().SetTimer(FSpreadCrossHairTimer, this, &UTPSCrossHair::SetCrossHairPos, GetWorld()->GetDeltaSeconds(), true);
	}
	LastSpreadValue = GetDynamicSpreadValue();
}

float UTPSCrossHair::GetDynamicSpreadValue()
{
	if(!MyOwner)
	{
		return 0.f;
	}
	
	if(MyOwner->GetIsDied())
	{
		return 0.f;
	}
	else
	{
		float TempRate = 1.f;
		if(MyOwner->GetIsFiring())
		{
			TempRate *= 2.5f;
		}
		if(MyOwner->GetVelocity().Size2D()>0.f)
		{
			TempRate *= 4.f;
		}
		if(MyOwner->GetIsAiming())
		{
			TempRate *= 0.1f;
		}
		if(MyOwner->GetCharacterMovement() && MyOwner->GetCharacterMovement()->IsFalling())
		{
			TempRate *= 10.f;
		}
		return FMath::Clamp(TempRate, 0.f, MaxSpreadRate) * SpreadUnitQuantity ;
	}
}

float UTPSCrossHair::GetSpreadSpeed()
{
	return MaxSpreadRate*SpreadUnitQuantity / ZeroToMaxSpreadTime ;
}

bool UTPSCrossHair::CheckIsCrossHairHasSpreadOrShrunk(UImage* CrossHair, bool IsLeftOrRight)
{
	const float AimPos = DefaultPosValue + GetDynamicSpreadValue();
	const FVector2D SlotPos = CrossHair ? UWidgetLayoutLibrary::SlotAsCanvasSlot(CrossHair)->GetPosition() : FVector2D() ;
	const float TempCrossHairPos = IsLeftOrRight ? SlotPos.X : SlotPos.Y;

	//检查准星是否扩张或者缩小过度
	return IsSpreading ? AimPos <= FMath::Abs(TempCrossHairPos) : AimPos >= FMath::Abs(TempCrossHairPos);
}

void UTPSCrossHair::SetCrossHairLineNewPos(UImage* CrossHair, ECrossHairPos CrossHairPos)
{
	if(!CrossHair)
	{
		return;
	}
	
	const float CurCrossPos = GetCrossHairPos(CrossHair);
	float SpreadValue;
	FVector2D NewAddPos;
	if(IsSpreading)
	{
		//虽然限制了范围，但位置也不是准确的，还是要手动修复
		SpreadValue = FMath::Clamp(GetSpreadSpeed()*GetWorld()->GetDeltaSeconds(), 0.f, MaxSpreadRate*SpreadUnitQuantity - CurCrossPos);
	}
	else
	{
		//虽然限制了范围，但位置也不是准确的，还是要手动修复
		SpreadValue = FMath::Clamp(GetSpreadSpeed()*GetWorld()->GetDeltaSeconds(), 0.f, CurCrossPos - DefaultPosValue);
	}

	switch (CrossHairPos)
	{
	case Left:
		NewAddPos = FVector2D(SpreadValue * -1, 0);
		break;
	case Right:
		NewAddPos = FVector2D(SpreadValue * 1, 0);
		break;
	case Up:
		NewAddPos = FVector2D(0, SpreadValue * -1);
		break;
	case Down:
		NewAddPos = FVector2D(0, SpreadValue * 1);
		break;
	default:
		break;
	}

	NewAddPos = IsSpreading ? NewAddPos : NewAddPos * -1;

	const FVector2D CurPosVector = UWidgetLayoutLibrary::SlotAsCanvasSlot(CrossHair)->GetPosition();
	UWidgetLayoutLibrary::SlotAsCanvasSlot(CrossHair)->SetPosition(CurPosVector + NewAddPos);
}

void UTPSCrossHair::SetCrossHairPos()
{
	bool IsLeftOrRight;
	UImage* ValidCrossHairLine = GetValidCrossHairLine(IsLeftOrRight);
	if(CheckIsCrossHairHasSpreadOrShrunk(ValidCrossHairLine, IsLeftOrRight))
	{
		//如果准星扩张或者缩小过度就修复位置
		GetWorld()->GetTimerManager().ClearTimer(FSpreadCrossHairTimer);
		FixAllCrossHairPos();
		return;
	}

	SetCrossHairLineNewPos(C_Left, ECrossHairPos::Left);
	SetCrossHairLineNewPos(C_Right, ECrossHairPos::Right);
	SetCrossHairLineNewPos(C_Up, ECrossHairPos::Up);
	SetCrossHairLineNewPos(C_Down, ECrossHairPos::Down);
}

void UTPSCrossHair::FixAllCrossHairPos()
{
	const FVector2D LeftPos  = FVector2D(DefaultPosValue * -1 + GetDynamicSpreadValue() * -1, 0);
	const FVector2D RightPos = FVector2D(DefaultPosValue * 1 + GetDynamicSpreadValue() * 1, 0);
	const FVector2D UpPos    = FVector2D(0,DefaultPosValue * -1 + GetDynamicSpreadValue() * -1);
	const FVector2D DownPos  = FVector2D(0,DefaultPosValue * 1 + GetDynamicSpreadValue() * 1);
	UWidgetLayoutLibrary::SlotAsCanvasSlot(C_Left)->SetPosition(LeftPos);
	UWidgetLayoutLibrary::SlotAsCanvasSlot(C_Right)->SetPosition(RightPos);
	UWidgetLayoutLibrary::SlotAsCanvasSlot(C_Up)->SetPosition(UpPos);
	UWidgetLayoutLibrary::SlotAsCanvasSlot(C_Down)->SetPosition(DownPos);
}
