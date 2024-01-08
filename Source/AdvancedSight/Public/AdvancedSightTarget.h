// Copyright 2024, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AdvancedSightTarget.generated.h"

UINTERFACE()
class UAdvancedSightTarget : public UInterface
{
	GENERATED_BODY()
};

class ADVANCEDSIGHT_API IAdvancedSightTarget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent)
	void GetVisibilityPoints(TArray<FVector>& OutVisibilityPoints) const;
};
