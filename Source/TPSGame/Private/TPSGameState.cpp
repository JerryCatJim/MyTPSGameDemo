// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSGameState.h"

void ATPSGameState::SortPlayerRank_Stable(UPARAM(ref)TArray<FPlayerDataInGame>& PlayerDataArray, bool HighToLow)
{
	PlayerDataArray.StableSort(
		[=](const FPlayerDataInGame& B, const FPlayerDataInGame& A)->bool //第一个参数是Array[Index + 1], 第二个参数才是Array[Index]
		{
			return HighToLow ? A.PersonalScore < B.PersonalScore : A.PersonalScore > B.PersonalScore;
		});

	for(int i = 0; i < PlayerDataArray.Num(); ++i)
	{
		PlayerDataArray[i].RankInGame = i + 1;
	}
}
