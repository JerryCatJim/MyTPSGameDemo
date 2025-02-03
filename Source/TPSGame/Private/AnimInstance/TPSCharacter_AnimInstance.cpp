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

	if(EquippedWeapon) { EquippedWeaponType = EquippedWeapon->GetWeaponType(); }
	
	bUseCommonLeftHandIK = !PlayerCharacter->GetIsReloading() && !PlayerCharacter->GetIsSwappingWeapon() &&
		!(EquippedWeaponType == ThrowGrenade || EquippedWeaponType == Knife || EquippedWeaponType == Fist);
	bUsePistolRightHandIK = EquippedWeaponType == Pistol && !PlayerCharacter->GetIsReloading();
	
	if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMeshComp() && PlayerCharacter->GetMesh())
	{
#pragma region HandIK
		//武器开火或者瞄准时会调用hand_r旋转朝向瞄准位置，左手可能错位，所以IK贴合(Reload时调用动画，暂停左手IK)
		if(bUseCommonLeftHandIK)
		{
			LeftHandTransToHoldWeapon = EquippedWeapon->GetWeaponMeshComp()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
			//微调一下位置
			if(EquippedWeaponType == Rifle && !(PlayerCharacter->GetIsAiming() || PlayerCharacter->GetIsFiring()))
			{
				LeftHandTransToHoldWeapon.SetLocation(LeftHandTransToHoldWeapon.GetLocation() + FVector(0, -2, 2));
			}
			FVector LOutLocation;
			FRotator LOutRotation;
			
			//获取LeftHandSocket在骨骼空间中相对middle_02_r的位置(因为动画蓝图中FABRIK选择的BoneSpace)
			//选middle_02_r作为参照是因为hand_r在瞄准时的位置和静止时差别较大，IK会出问题
			PlayerCharacter->GetMesh()->TransformToBoneSpace(FName("middle_02_r"), LeftHandTransToHoldWeapon.GetLocation(), FRotator::ZeroRotator, LOutLocation, LOutRotation);
			LeftHandTransToHoldWeapon.SetLocation(LOutLocation);
			LeftHandTransToHoldWeapon.SetRotation(FQuat(LOutRotation));
			//设置好新位置后IK位置也不一定是理想位置，因为FABRIK中设置的No Change Rotation(其余两个选项试了半天也没玩转)，所以位置可能有偏差，就辛苦辛苦在Mesh上多改改Socket位置吧。
		}
		
		if(bUsePistolRightHandIK)
		{
			//手枪暂时缺少射击动画资源，射击时右手反而没有伸出来，所以改为调用右手IK,把右手伸到左手的位置
			
			//持手枪未射击 未瞄准 未换弹 时，把左手伸到LeftHandPistol_Idle_Socket处(因为缺动画)
			bUsePistolLeftHandIK_NoAim = !PlayerCharacter->GetIsAiming() && !PlayerCharacter->GetIsFiring() && !PlayerCharacter->GetIsReloading();
			if(bUsePistolLeftHandIK_NoAim)
			{
				//左手IK不在结尾时再修改了
				bUseCommonLeftHandIK = false;
				
				LeftHandTransOfPistol_NoAim = PlayerCharacter->GetMesh()->GetSocketTransform(FName("LeftHandPistol_Idle_Socket"), ERelativeTransformSpace::RTS_World);
				FVector LOutLocation;
				FRotator LOutRotation;
				
				PlayerCharacter->GetMesh()->TransformToBoneSpace(
					FName("spine_01"),
					LeftHandTransOfPistol_NoAim.GetLocation(),
					FRotator::ZeroRotator,
					LOutLocation,
					LOutRotation);
				LeftHandTransOfPistol_NoAim.SetLocation(LOutLocation);
				LeftHandTransOfPistol_NoAim.SetRotation(FQuat(LOutRotation));
			}
			
			RightHandTransToMoveToLeftHand = PlayerCharacter->GetMesh()->GetSocketTransform(FName("RightHandPistolSocket"), ERelativeTransformSpace::RTS_World);
			FVector ROutLocation;
			FRotator ROutRotation;
			PlayerCharacter->GetMesh()->TransformToBoneSpace(
				FName("middle_01_l"),
				RightHandTransToMoveToLeftHand.GetLocation(),
				FRotator::ZeroRotator,
				ROutLocation,
				ROutRotation);
			RightHandTransToMoveToLeftHand.SetLocation(ROutLocation);
			RightHandTransToMoveToLeftHand.SetRotation(FQuat(ROutRotation));
		}
#pragma endregion
	}
}