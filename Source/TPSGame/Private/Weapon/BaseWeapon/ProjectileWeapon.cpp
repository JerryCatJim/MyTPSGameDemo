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

	if(!SplineComponent)
	{
		SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	}
	if(SplineComponent)
	{
		SplineComponent->ClearSplinePoints();
	}
	/*if(GetRootComponent() && Cast<USkeletalMeshComponent>(GetRootComponent()))
	{
		//别在子类直接Attach到Root，有可能自己变成Root然后导致命名冲突而直接崩溃
		SplineComponent->SetupAttachment(GetRootComponent());
	}*/
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
	if(SplineComponent)
	{
		SplineComponent->ClearSplinePoints();
	}
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
			TWeakObjectPtr<AProjectile> SpawnProjectile = World->SpawnActorDeferred<AProjectile>(
				ProjectileClass,
				SpawnTransform,
				this,
				InstigatorPawn
			);
			if(SpawnProjectile.IsValid())
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

	if(MyOwner->GetCurrentWeapon() != this || MyOwner->GetIsSwappingWeapon() ) return;
	
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
	
	if((OnlyDrawTrajectoryWhenAiming && MyOwner->GetIsAiming() && !MyOwner->GetIsReloading())
		|| !OnlyDrawTrajectoryWhenAiming )
	{
		FHitResult HitResult;
		TArray<FVector> OutPoints;
		FVector LastTraceDestination;
		FVector StartPos = MuzzleSocket->GetSocketLocation(GetWeaponMeshComp());
		
		FVector LaunchVelocity(((GetCurrentAimingPoint(!KeepTrajectoryStable) - StartPos).Rotation()+FRotator(UpAngelOffset,0,0)).Vector().GetSafeNormal() * ProjectileInitialSpeed);
		FVector WeaponSocketVelocity = MuzzleSocket->GetSocketTransform(GetWeaponMeshComp()).GetUnitAxis( EAxis::X ).GetSafeNormal()* ProjectileInitialSpeed;
		//如果常驻显示轨迹，未瞄准或开火,换弹时希望速度方向为枪口方向
		LaunchVelocity = !(MyOwner->GetIsAiming() || MyOwner->GetIsFiring()) || MyOwner->GetIsReloading() ? WeaponSocketVelocity : LaunchVelocity ;
		
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Emplace(ECC_WorldStatic);
		//ObjectTypes.Emplace(ECC_WorldDynamic);
		ObjectTypes.Emplace(ECC_Pawn);
		
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Emplace(MyOwner);
		
		//初速度太快则适当缩短预测距离
		float RealDrawTrajectoryTime = ProjectileInitialSpeed >= 3000 ? DrawTrajectoryTime * 3000/ProjectileInitialSpeed : DrawTrajectoryTime;
		
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
			RealDrawTrajectoryTime,
			GetWorld()->GetWorldSettings()->WorldGravityZ * TrajectoryGravityZScale
			);

		if(TrajectoryTargetPointClass)
		{
			if(!TrajectoryTargetPointActor && GetWorld())
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
			/*if(!SplineComponent)
			{
				SplineComponent = NewObject<USplineComponent>(this, TEXT("FUCKU"));
				//生成失败则返回
				if(!SplineComponent) return;
				
				AddOwnedComponent(SplineComponent);
				SplineComponent->SetupAttachment(GetRootComponent());
				SplineComponent->RegisterComponent();
			}*/
			if(!SplineComponent)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0 ,FColor::Red, TEXT("SplineComponent不存在!"));
				return;
			}
			
			for(int i = 0; i < OutPoints.Num(); i += 1)
			{
				SplineComponent->AddSplinePointAtIndex(OutPoints[i], i, ESplineCoordinateSpace::World);
			}
			SplineComponent->SetSplinePointType(OutPoints.Num()-1, ESplinePointType::CurveClamped);
			
			for(int i = 0; i < OutPoints.Num()-1 ; i += DrawTrajectoryJumpNum <= 0 ? 1 : DrawTrajectoryJumpNum)
			{
				TWeakObjectPtr<USplineMeshComponent> Spl = NewObject<USplineMeshComponent>(SplineComponent, TEXT("SplineMesh" + i));
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
				
				TrajectoryLineArray.Emplace(Spl.Get());
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
			//DestoryComponent会自动调用UnregisterComponent()
			//Item->UnregisterComponent();
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
	if(SplineComponent)
	{
		SplineComponent->ClearSplinePoints();
		SplineComponent->DestroyComponent();
	}
}
