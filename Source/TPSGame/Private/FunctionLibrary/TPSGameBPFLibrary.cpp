// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionLibrary/TPSGameBPFLibrary.h"
#include "Kismet/GameplayStatics.h"

bool UTPSGameBPFLibrary::IsInScreenViewport(const UObject* WorldContextObject, const FVector& WorldPosition)
{
	APlayerController *Player = UGameplayStatics::GetPlayerController(WorldContextObject,0);
	ULocalPlayer* const LP = Player ? Player->GetLocalPlayer() : nullptr;
	if (LP && LP->ViewportClient)
	{
		// get the projection data
		FSceneViewProjectionData ProjectionData;
		if (LP->GetProjectionData(LP->ViewportClient->Viewport, eSSP_FULL, /*out*/ ProjectionData))
		{
			FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
			FVector2D ScreenPosition;
			bool bResult = FSceneView::ProjectWorldToScreen(WorldPosition, ProjectionData.GetConstrainedViewRect(), ViewProjectionMatrix, ScreenPosition);
			if (bResult && ScreenPosition.X > ProjectionData.GetViewRect().Min.X && ScreenPosition.X < ProjectionData.GetViewRect().Max.X
				&& ScreenPosition.Y > ProjectionData.GetViewRect().Min.Y && ScreenPosition.Y < ProjectionData.GetViewRect().Max.Y)
			{
				return true;
			}
		}
	}
	return false;
}
