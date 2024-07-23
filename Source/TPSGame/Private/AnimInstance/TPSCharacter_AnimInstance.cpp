// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/TPSCharacter_AnimInstance.h"

#include "SCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UTPSCharacter_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerCharacter = Cast<ASCharacter>(TryGetPawnOwner());
}

void UTPSCharacter_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if(!IsValid(PlayerCharacter))
	{
		PlayerCharacter = Cast<ASCharacter>(TryGetPawnOwner());
	}

	if(!IsValid(PlayerCharacter))
	{
		return;
	}	
	bWeaponEquipped = IsValid(PlayerCharacter->CurrentWeapon);
	EquippedWeapon = bWeaponEquipped ? PlayerCharacter->CurrentWeapon : nullptr;
	
	bIsInAir = PlayerCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = PlayerCharacter->GetVelocity().Size() > 0.f;
	Speed = PlayerCharacter->GetVelocity().Size2D();
	DirectionDegree = CalculateDirection(PlayerCharacter->GetVelocity(), PlayerCharacter->GetActorRotation());

	AimOffset_Y = PlayerCharacter->GetAimOffset_Y();
	AimOffset_Z = PlayerCharacter->GetAimOffset_Z();

	if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMeshComp() && PlayerCharacter->GetMesh())
	{
		EquippedWeaponType = EquippedWeapon->GetWeaponType();

		if(EquippedWeaponType == EWeaponType::Pistol)
		{
			//效果不好，已放弃手部IK
			if(!(PlayerCharacter->GetIsAiming() || PlayerCharacter->GetIsFiring()))
			{
				//未射击或者瞄准时，拿手枪的左手不在武器上，算出应在的位置然后对左手应用IK
				LeftHandTransToHoldWeapon = EquippedWeapon->GetWeaponMeshComp()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
				FVector LOutLocation;
				FRotator LOutRotation;
				PlayerCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransToHoldWeapon.GetLocation(), FRotator::ZeroRotator, LOutLocation, LOutRotation);
				LeftHandTransToHoldWeapon.SetLocation(LOutLocation);
				LeftHandTransToHoldWeapon.SetRotation(FQuat(LOutRotation));
			}

			//效果不好，已放弃手部IK
			if(PlayerCharacter->GetIsAiming() || PlayerCharacter->GetIsFiring())
			{
				//手枪开火时没有匹配动画，右手会因为Rifle开火动画往后缩，把右手用IK挪到左手的位置
				RightHandTransToMoveToLeftHand = PlayerCharacter->GetMesh()->GetSocketTransform(FName("RightHandToMoveSocket"), ERelativeTransformSpace::RTS_World);
				FVector ROutLocation;
				FRotator ROutRotation;
				PlayerCharacter->GetMesh()->TransformToBoneSpace(FName("hand_l"), RightHandTransToMoveToLeftHand.GetLocation(), FRotator::ZeroRotator, ROutLocation, ROutRotation);
				RightHandTransToMoveToLeftHand.SetLocation(ROutLocation);
				RightHandTransToMoveToLeftHand.SetRotation(FQuat(ROutRotation));
			}
		}
	}
}
