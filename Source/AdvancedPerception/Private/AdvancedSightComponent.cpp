// Copyright 2023, Robert Lewicki, All rights reserved.


#include "AdvancedSightComponent.h"

#include "AdvancedSightSystem.h"

UAdvancedSightComponent::UAdvancedSightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

FTransform UAdvancedSightComponent::GetEyePointOfViewTransform() const
{
	if (const auto* AIController = GetOwner<AController>())
	{
		return AIController->GetPawn()->GetActorTransform();
	}

	return GetOwner()->GetTransform();
}

AActor* UAdvancedSightComponent::GetBodyActor() const
{
	if (const auto* AIController = GetOwner<AController>())
	{
		return AIController->GetPawn();
	}

	return GetOwner();
}

void UAdvancedSightComponent::SpotTarget(AActor* TargetActor)
{
	OnTargetSpotted.Broadcast(TargetActor);
}

void UAdvancedSightComponent::PerceiveTarget(AActor* TargetActor)
{
	PerceivedTargets.Add(TargetActor);
	OnTargetPerceived.Broadcast(TargetActor);
}

void UAdvancedSightComponent::LoseTarget(AActor* TargetActor)
{
	PerceivedTargets.Remove(TargetActor);
	OnTargetLost.Broadcast(TargetActor);
}

TSet<TObjectPtr<AActor>> UAdvancedSightComponent::GetPerceivedTargets() const
{
	return PerceivedTargets;
}

bool UAdvancedSightComponent::IsTargetPerceived(const AActor* TargetActor) const
{
	return PerceivedTargets.Contains(TargetActor);
}

void UAdvancedSightComponent::BeginPlay()
{
	Super::BeginPlay();

	if (auto* AdvancedSightSystem = GetWorld()->GetSubsystem<UAdvancedSightSystem>())
	{
		AdvancedSightSystem->RegisterListener(this);
	}
}


void UAdvancedSightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		if (auto* AdvancedSightSystem = World->GetSubsystem<UAdvancedSightSystem>())
		{
			AdvancedSightSystem->UnregisterListener(this);
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

UAdvancedSightData* UAdvancedSightComponent::GetSightData() const
{
	return SightData;
}

void UAdvancedSightComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
