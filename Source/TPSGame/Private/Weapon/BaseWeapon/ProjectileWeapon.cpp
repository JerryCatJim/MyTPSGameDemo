// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/BaseWeapon/ProjectileWeapon.h"
#include "Weapon/Projectile/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"  //射线检测显示颜色
#include "SCharacter.h"
#include "Kismet/GameplayStatics.h"

AProjectileWeapon::AProjectileWeapon()
{
	WeaponBulletType = EWeaponBulletType::Projectile;
}

void AProjectileWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawMovementTrajectory();
}

void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	MuzzleSocket = GetWeaponMeshComp()->GetSocketByName(MuzzleSocketName);
}

void AProjectileWeapon::DealFire()
{
	//Super::DealFire();
	//与射线检测的即时命中武器不同，发射器武器完全重写发射逻辑

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	//const USkeletalMeshSocket* MuzzleSocket = GetWeaponMeshComp()->GetSocketByName(MuzzleSocketName);

	//没有MuzzleSocket的武器无法发射子弹
	if(!MuzzleSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red,
					FString::Printf(TEXT("当前武器未设置MuzzleFlash位置!")));
		return;
	}
	
	FTransform SocketTransform = MuzzleSocket->GetSocketTransform(GetWeaponMeshComp());
	//枪口 指向 准星瞄准方向的终点 的向量
	FVector ToTarget = GetCurrentAimingPoint() - SocketTransform.GetLocation();
	FRotator TargetRotation = FRotator(ToTarget.Rotation().Pitch + UpAngelOffset, ToTarget.Rotation().Yaw, ToTarget.Rotation().Roll);
	if(ProjectileClass && InstigatorPawn)
	{
		UWorld* World = GetWorld();
		if(World)
		{
			FTransform SpawnTransform = FTransform(TargetRotation,SocketTransform.GetLocation());
			AProjectile* SpawnProjectile = World->SpawnActorDeferred<AProjectile>(
				ProjectileClass,
				SpawnTransform,
				this,
				InstigatorPawn
			);
			if(SpawnProjectile)
			{
				if(OverrideProjectileDefaultData)
				{
					SpawnProjectile->GravityZScale = ProjectileGravityZScale;
					SpawnProjectile->InitialSpeed = ProjectileInitialSpeed;
					SpawnProjectile->MaxSpeed = ProjectileMaxSpeed;
					SpawnProjectile->WasOverrideFromWeapon = true;
				}
				SpawnProjectile->FinishSpawning(SpawnTransform);
			}
		}
	}

	//DrawDebugLine(GetWorld(), SocketTransform.GetLocation(), GetCurrentAimingPoint(), FColor::Red, false, 1.0f, 0,1.0f);
}

void AProjectileWeapon::DrawMovementTrajectory()
{
	if(!CanShowMovementTrajectory || !IsValid(MyOwner) || MyOwner->GetIsDied()) return;

	if(MyOwner->CurrentWeapon != this) return;
	
	if(DrawTrajectoryLocally && !MyOwner->IsLocallyControlled()) return;
	
	//没有MuzzleSocket的武器无法发射子弹
	if(!MuzzleSocket)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red,
					FString::Printf(TEXT("当前武器未设置MuzzleFlash位置!")));
		return;
	}
	if(!OverrideProjectileDefaultData)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red,
					FString::Printf(TEXT("武器中的速度和重力系数未覆盖给子弹，无法描绘预测轨迹!")));
		return;
	}
	
	if(MyOwner->GetIsAiming() && !MyOwner->GetIsReloading())
	{
		FHitResult HitResult;
		TArray<FVector> OutPoints;
		FVector LastTraceDestination;
		FVector StartPos = MuzzleSocket->GetSocketLocation(GetWeaponMeshComp());
		FVector LaunchVelocity(((GetCurrentAimingPoint(false) - StartPos).Rotation()+FRotator(UpAngelOffset,0,0)).Vector().GetSafeNormal() * ProjectileInitialSpeed);
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Emplace(ECC_WorldStatic);
		ObjectTypes.Emplace(ECC_WorldDynamic);
		ObjectTypes.Emplace(ECC_Pawn);
		TArray<AActor*> ActorsToIgnore;
		UGameplayStatics::Blueprint_PredictProjectilePath_ByObjectType(
			this,
			HitResult,
			OutPoints,
			LastTraceDestination,
			StartPos,
			LaunchVelocity,
			true,
			3,
			ObjectTypes,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::ForOneFrame,
			0,
			DrawFrequency,
			DrawTrajectoryTime,
			GetWorld()->GetWorldSettings()->WorldGravityZ * ProjectileGravityZScale
			);
	}
}
