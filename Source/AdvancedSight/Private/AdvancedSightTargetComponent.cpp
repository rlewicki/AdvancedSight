// Copyright 2024, Robert Lewicki, All rights reserved.


#include "AdvancedSightTargetComponent.h"

#include "AdvancedSightTarget.h"

UAdvancedSightTargetComponent::UAdvancedSightTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

const TArray<USceneComponent*>& UAdvancedSightTargetComponent::GetVisibilityPointComponents() const
{
	return VisibilityPointComponents;
}

void UAdvancedSightTargetComponent::GetVisibilityPoints(TArray<FVector>& VisibilityPoints) const
{
	VisibilityPoints.Reserve(VisibilityPointComponents.Num());
	for (const USceneComponent* VisibilityPointComponent : VisibilityPointComponents)
	{
		VisibilityPoints.Add(VisibilityPointComponent->GetComponentLocation());
	}
}

void UAdvancedSightTargetComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (ensure(GetOwner()->Implements<UAdvancedSightTarget>()))
    {
    	IAdvancedSightTarget::Execute_GetVisibilityPointComponents(GetOwner(), VisibilityPointComponents);
    }
}
