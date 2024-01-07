// Copyright 2024, Robert Lewicki, All rights reserved.

#include "AdvancedSightComponent.h"

#include "AdvancedSightCommon.h"
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
	SpottedTargets.Add(TargetActor);
	OnTargetSpotted.Broadcast(TargetActor);
}

void UAdvancedSightComponent::PerceiveTarget(AActor* TargetActor)
{
	SpottedTargets.RemoveSwap(TargetActor);
	PerceivedTargets.Add(TargetActor);
	OnTargetPerceived.Broadcast(TargetActor);
}

void UAdvancedSightComponent::LoseTarget(AActor* TargetActor)
{
	const int32 RemovedNum = PerceivedTargets.RemoveSwap(TargetActor);
	if (RemovedNum == 0)
	{
		SpottedTargets.RemoveSwap(TargetActor);	
	}
	else
	{
		RememberedTargets.Add(TargetActor);
	}
	
	OnTargetLost.Broadcast(TargetActor);
}

void UAdvancedSightComponent::ForgetTarget(AActor* TargetActor)
{
	RememberedTargets.RemoveSwap(TargetActor);
	OnTargetForgot.Broadcast(TargetActor);
}

const TArray<AActor*>& UAdvancedSightComponent::GetPerceivedTargets() const
{
	return PerceivedTargets;
}

const TArray<AActor*>& UAdvancedSightComponent::GetSpottedTargets() const
{
	return SpottedTargets;
}

const TArray<AActor*>& UAdvancedSightComponent::GetRememberedTargets() const
{
	return RememberedTargets;
}

bool UAdvancedSightComponent::IsTargetPerceived(const AActor* TargetActor) const
{
	return PerceivedTargets.Contains(TargetActor);
}

float UAdvancedSightComponent::GetGainValueForTarget(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		UE_LOG(
			LogAdvancedSight,
			Warning,
			TEXT("%s: Invalid target actor reference"), StringCast<TCHAR>(__FUNCTION__).Get());
		return -1.0f;
	}

	const float GainValue =
			AdvancedSightSystem->GetGainValueForTarget(GetUniqueID(), TargetActor->GetUniqueID());
	return GainValue;
}

void UAdvancedSightComponent::BeginPlay()
{
	Super::BeginPlay();

	AdvancedSightSystem = GetWorld()->GetSubsystem<UAdvancedSightSystem>();
	if (ensureMsgf(AdvancedSightSystem.IsValid(), TEXT("Advanced sight system reference is invalid")))
	{
		AdvancedSightSystem->RegisterListener(this);
	}
}


void UAdvancedSightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AdvancedSightSystem.IsValid())
	{
		AdvancedSightSystem->UnregisterListener(this);
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
