// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon/BaseWeapon/ProjectileWeapon.h"
#include "Weapon/Projectile/Projectile.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"  //射线检测显示颜色
#include "SCharacter.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileWeapon::AProjectileWeapon()
{
	WeaponBulletType = EWeaponBulletType::Projectile;
}

void AProjectileWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ClearMovementTrajectory();
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
					SpawnProjectile->InnerRadius = ProjectileInnerRadius;
					SpawnProjectile->OuterRadius = ProjectileOuterRadius;
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
	
	if((OnlyDrawTrajectoryWhenReloading && MyOwner->GetIsAiming() && !MyOwner->GetIsReloading())
		|| !OnlyDrawTrajectoryWhenReloading )
	{
		FHitResult HitResult;
		TArray<FVector> OutPoints;
		FVector LastTraceDestination;
		FVector StartPos = MuzzleSocket->GetSocketLocation(GetWeaponMeshComp());
		FVector LaunchVelocity(((GetCurrentAimingPoint(!KeepTrajectoryStable) - StartPos).Rotation()+FRotator(UpAngelOffset,0,0)).Vector().GetSafeNormal() * ProjectileInitialSpeed);
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Emplace(ECC_WorldStatic);
		//ObjectTypes.Emplace(ECC_WorldDynamic);
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
			EDrawDebugTrace::None,
			0,
			DrawFrequency,
			DrawTrajectoryTime,
			GetWorld()->GetWorldSettings()->WorldGravityZ * ProjectileGravityZScale
			);

		if(TrajectoryTargetPointClass)
		{
			if(!TrajectoryTargetPointActor)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = this;  //要在Spawn时就指定Owner，否则没法生成物体后直接在其BeginPlay中拿到Owner，会为空
				TrajectoryTargetPointActor = GetWorld()->SpawnActor<AActor>(TrajectoryTargetPointClass,FVector(),FRotator(),SpawnParams);
			}
			if(TrajectoryTargetPointActor && TrajectoryTargetPointActor->GetRootComponent() && !TrajectoryTargetPointActor->GetRootComponent()->IsVisible())
			{
				TrajectoryTargetPointActor->SetActorLocation(LastTraceDestination);
				TrajectoryTargetPointActor->SetActorRotation(FRotator(0,0,0));
				TrajectoryTargetPointActor->GetRootComponent()->SetVisibility(true,true);
			}
		}
		if(TrajectoryLineMesh)
		{
			if(!SplineComponent)
			{
				SplineComponent = NewObject<USplineComponent>(this, TEXT("SplineComponent"));
				//生成失败则返回
				if(!SplineComponent) return;
				
				AddOwnedComponent(SplineComponent);
				SplineComponent->SetupAttachment(GetRootComponent());
				SplineComponent->RegisterComponent();
			}
			
			for(int i = 0; i < OutPoints.Num(); i += 1)
			{
				SplineComponent->AddSplinePointAtIndex(OutPoints[i], i, ESplineCoordinateSpace::World);
			}
			SplineComponent->SetSplinePointType(OutPoints.Num()-1, ESplinePointType::CurveClamped);
			
			for(int i = 0; i < OutPoints.Num()-1 ; i += DrawTrajectoryJumpNum <= 0 ? 1 : DrawTrajectoryJumpNum)
			{
				USplineMeshComponent* Spl = NewObject<USplineMeshComponent>(this, TEXT("SplineMesh" + i));
				Spl->RegisterComponent();
				//默认生成的是Static
				Spl->SetMobility(EComponentMobility::Movable);
				Spl->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Spl->SetStaticMesh(TrajectoryLineMesh);
				Spl->SetForwardAxis(ESplineMeshAxis::Z);

				FVector StartTangent = SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
				FVector EndTangent = SplineComponent->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::World);
				Spl->SetStartAndEnd(OutPoints[i],StartTangent,OutPoints[i+1],EndTangent);
				Spl->SetStartScale(FVector2D(TrajectoryLineScale,TrajectoryLineScale));
				Spl->SetEndScale(FVector2D(TrajectoryLineScale,TrajectoryLineScale));
				
				TrajectoryLineArray.Emplace(Spl);
			}
		}
	}
}

void AProjectileWeapon::ClearMovementTrajectory()
{
	if(SplineComponent)
	{
		SplineComponent->ClearSplinePoints();
	}
	if(TrajectoryLineArray.Num() > 0)
	{
		for(auto& Item : TrajectoryLineArray)
		{
			Item->UnregisterComponent();
			Item->DestroyComponent();
		}
		TrajectoryLineArray.Empty();
	}
	if(TrajectoryTargetPointActor && TrajectoryTargetPointActor->GetRootComponent() && TrajectoryTargetPointActor->GetRootComponent()->IsVisible())
	{
		TrajectoryTargetPointActor->GetRootComponent()->SetVisibility(false, true);
	}
}

void AProjectileWeapon::Destroyed()
{
	ClearMovementTrajectory();
}
