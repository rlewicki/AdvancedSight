// Copyright 2023, Robert Lewicki, All rights reserved.

#include "AdvancedSightSystem.h"

#include "AdvancedSightComponent.h"
#include "AdvancedSightData.h"
#include "AdvancedSightTarget.h"

void UAdvancedSightSystem::RegisterListener(UAdvancedSightComponent* SightComponent)
{
	Listeners.Add(SightComponent->GetUniqueID(), SightComponent);
	for (const TTuple<unsigned, TWeakObjectPtr<AActor>>& TargetActor : TargetActors)
	{
		AddQuery(SightComponent, TargetActor.Value.Get(), SightComponent->GetSightData());
	}
}

void UAdvancedSightSystem::UnregisterListener(UAdvancedSightComponent* SightComponent)
{
	Queries.RemoveAllSwap([SightComponent](const FAdvancedSightQuery& Query)
	{
		return SightComponent->GetUniqueID() == Query.ListenerId;
	});
}

void UAdvancedSightSystem::RegisterTarget(AActor* TargetActor)
{
	if (!TargetActor->Implements<UAdvancedSightTarget>())
	{
		return;
	}

	TargetActors.Add(TargetActor->GetUniqueID(), TargetActor);
	for (const TTuple<unsigned, TWeakObjectPtr<UAdvancedSightComponent>>& Listener : Listeners)
	{
		AddQuery(Listener.Value.Get(), TargetActor, Listener.Value->GetSightData());
	}
}

void UAdvancedSightSystem::UnregisterTarget(AActor* TargetActor)
{
	Queries.RemoveAllSwap([TargetActor](const FAdvancedSightQuery& Query)
	{
		return TargetActor->GetUniqueID() == Query.TargetId;
	});
}

void UAdvancedSightSystem::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		const UWorld* World = GetWorld();
		const FOnActorSpawned::FDelegate ActorSpawnedDelegate = FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleNewActorSpawned);
		NewActorSpawnedDelegateHandle = World->AddOnActorSpawnedHandler(ActorSpawnedDelegate);
	}
}

void UAdvancedSightSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UAdvancedSightSystem::Tick");

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FAdvancedSightQuery& Query : Queries)
	{
		UAdvancedSightComponent* SightComponent = Listeners[Query.ListenerId].Get();
		const FTransform ListenerTransform = SightComponent->GetEyePointOfViewTransform();
		AActor* TargetActor = TargetActors[Query.TargetId].Get();
		const FVector TargetLocation = TargetActor->GetActorLocation();
		bool bIsVisible = false;
		float BestGainMultiplier = 0.0f;
		for (const FAdvancedSightInfo& SightInfo : Query.SightInfos)
		{
			const float CheckRadiusSq = FMath::Square(
				Query.bIsTargetPerceived
				? SightInfo.LoseRadius
				: SightInfo.GainRadius);
			const float DistanceSq = FVector::DistSquared(ListenerTransform.GetLocation(), TargetLocation);
			if (DistanceSq > CheckRadiusSq)
			{
				continue;
			}

			const FVector DirectionToTarget = (TargetLocation - ListenerTransform.GetLocation()).GetSafeNormal();
			const FVector ListenerForward = ListenerTransform.GetRotation().Vector();
			const float DotProduct = FVector::DotProduct(DirectionToTarget, ListenerForward);
			const float Angle = FMath::Acos(DotProduct);
			const float MaxAngle = FMath::DegreesToRadians(SightInfo.FOV / 2.0f);
			if (Angle > MaxAngle)
			{
				continue;
			}

			FHitResult HitResult;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(SightComponent->GetBodyActor());
			const bool bHit = World->LineTraceSingleByChannel(
				HitResult,
				ListenerTransform.GetLocation(),
				TargetLocation,
				ECC_Visibility,
				QueryParams);
			if (!bHit)
			{
				continue;
			}

			if (HitResult.GetActor() == TargetActor)
			{
				bIsVisible = true;
				BestGainMultiplier = SightInfo.GainMultiplier;
			}
		}

		if (bIsVisible)
		{
			if (!Query.bWasLastCheckSuccess)
			{
				Query.bWasLastCheckSuccess = true;
				SightComponent->SpotTarget(TargetActor);
			}

			Query.LastSeenLocation = TargetLocation;
			if (!Query.bIsTargetPerceived)
			{
				Query.GainValue += DeltaTime * BestGainMultiplier;
				if (Query.GainValue > 1.0f)
				{
					Query.bIsTargetPerceived = true;
					SightComponent->PerceiveTarget(TargetActor);
				}
			}
			else
			{
				Query.LoseSightTimer = 0.0f;
			}
		}
		else
		{
			Query.bWasLastCheckSuccess = false;

			if (Query.bIsTargetPerceived)
			{
				Query.LoseSightTimer += DeltaTime;
				if (Query.LoseSightTimer >= Query.LoseSightCooldown)
				{
					Query.bIsTargetPerceived = false;
					SightComponent->LoseTarget(TargetActor);
				}
			}
			else
			{
				Query.GainValue -= DeltaTime;
				if (Query.GainValue < 0.0f)
				{
					Query.GainValue = 0.0f;
				}
			}
		}
	}
}

TStatId UAdvancedSightSystem::GetStatId() const
{
	return TStatId();
}

void UAdvancedSightSystem::HandleNewActorSpawned(AActor* Actor)
{
	RegisterTarget(Actor);
}

void UAdvancedSightSystem::AddQuery(
	const UAdvancedSightComponent* SightComponent, const AActor* TargetActor, const UAdvancedSightData* SightData)
{
	if (SightComponent->GetOwner() ==  TargetActor)
	{
		return;
	}

	FAdvancedSightQuery& Query = Queries.AddDefaulted_GetRef();
	Query.ListenerId = SightComponent->GetUniqueID();
	Query.TargetId = TargetActor->GetUniqueID();
	Query.SightInfos = SightData->SightInfos;
	Query.LoseSightCooldown = SightData->LoseSightCooldown;
}
