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

void UAdvancedSightComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}