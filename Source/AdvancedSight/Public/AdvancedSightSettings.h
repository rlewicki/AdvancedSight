// Copyright 2024, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AdvancedSightSettings.generated.h"

UCLASS(Config=Game, DefaultConfig)
class ADVANCEDSIGHT_API UAdvancedSightSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TEnumAsByte<ECollisionChannel> AdvancedSightCollisionChannel = ECC_WorldStatic;
};
