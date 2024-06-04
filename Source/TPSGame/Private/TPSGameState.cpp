// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameState.h"

#include "TPSPlayerState.h"

void ATPSGameState::SortPlayerRank_Stable(UPARAM(ref)TArray<ATPSPlayerState*>& PlayerStateArray, bool HighToLow)
{
	PlayerStateArray.StableSort(
		[=](const ATPSPlayerState& B, const ATPSPlayerState& A)->bool //第一个参数是Array[Index + 1], 第二个参数才是Array[Index]
		{
			return HighToLow ? A.PersonalScore < B.PersonalScore : A.PersonalScore > B.PersonalScore;
		});

	for(int i = 0; i < PlayerStateArray.Num(); ++i)
	{
		PlayerStateArray[i]->RankInGame = i + 1;
	}
}
