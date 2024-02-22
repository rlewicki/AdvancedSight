// Copyright 2024, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AdvancedSightSettings.generated.h"

USTRUCT(BlueprintType)
struct ADVANCEDSIGHT_API FDebugDrawInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FColor PerceivedTarget_VisibleColor = FColor::Green;

	UPROPERTY(EditDefaultsOnly)
	FColor PerceivedTarget_UnconfirmedColor = FColor(29, 140, 48);

	UPROPERTY(EditDefaultsOnly)
	FColor SpottedTarget_VisibleColor = FColor::Turquoise;

	UPROPERTY(EditDefaultsOnly)
	FColor SpottedTarget_UnconfirmedColor = FColor::Blue;

	UPROPERTY(EditDefaultsOnly)
	FColor NotPerceivedColor = FColor::Red;

	UPROPERTY(EditDefaultsOnly)
	FColor LastKnownLocationColor = FColor::Yellow;

	UPROPERTY(EditDefaultsOnly)
	float VisibilityPointRadius = 16.0f;

	UPROPERTY(EditDefaultsOnly)
	int32 VisibilityPointSphereSegments = 8;
};

UCLASS(Config=Game, DefaultConfig)
class ADVANCEDSIGHT_API UAdvancedSightSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	TEnumAsByte<ECollisionChannel> AdvancedSightCollisionChannel = ECC_WorldStatic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	FDebugDrawInfo DebugDrawInfo;
};
