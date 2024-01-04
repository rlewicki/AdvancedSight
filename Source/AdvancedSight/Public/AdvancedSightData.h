// Copyright 2023, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AdvancedSightData.generated.h"

USTRUCT(BlueprintType)
struct ADVANCEDSIGHT_API FAdvancedSightInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float GainRadius = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FOV = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float GainMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, AdvancedDisplay)
	FColor DebugColor = FColor::Green;
};

UCLASS()
class ADVANCEDSIGHT_API UAdvancedSightData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FAdvancedSightInfo> SightInfos;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float LoseSightRadius = 1250.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float LoseSightCooldown = 1.0f;
};
