// Copyright 2024, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AdvancedSightTargetComponent.generated.h"

class USceneComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ADVANCEDSIGHT_API UAdvancedSightTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAdvancedSightTargetComponent();
	const TArray<USceneComponent*>& GetVisibilityPointComponents() const;
	void GetVisibilityPoints(TArray<FVector>& VisibilityPoints) const;
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY(Transient)
	TArray<USceneComponent*> VisibilityPointComponents;
};
