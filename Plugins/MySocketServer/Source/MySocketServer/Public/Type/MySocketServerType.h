#pragma once
#include "CoreMinimal.h"

DECLARE_DELEGATE_TwoParams(FMyWebSocketReceiveCallBack, FGuid /*ClientId*/ , FString /*ReceiveMessage*/);
DECLARE_DELEGATE_OneParam(FMyWebSocketClosedCallBack, FGuid /*ClientId*/);