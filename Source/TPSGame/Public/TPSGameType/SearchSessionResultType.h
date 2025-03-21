#pragma once

#include "OnlineSessionSettings.h"
//#include "CoreMinimal.h"
#include "SearchSessionResultType.generated.h"

USTRUCT(BlueprintType)
struct FMySearchSessionResult
{
	GENERATED_USTRUCT_BODY();
	
public:
	FOnlineSessionSearchResult OnlineResult;

};