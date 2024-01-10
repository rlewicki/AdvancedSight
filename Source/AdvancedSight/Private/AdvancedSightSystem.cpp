// Copyright 2024, Robert Lewicki, All rights reserved.

#include "AdvancedSightSystem.h"

#include "AdvancedSightComponent.h"
#include "AdvancedSightData.h"
#include "AdvancedSightTarget.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<bool> ShouldDebugDraw(
	TEXT("AdvancedSight.ShouldDebugDraw"), false, TEXT("Set this to true to see the closest listener debug drawing"));

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

float UAdvancedSightSystem::GetGainValueForTarget(const uint32 ListenerId, const uint32 TargetId) const
{
	const FAdvancedSightQuery* Query = Queries.FindByPredicate([ListenerId, TargetId](const FAdvancedSightQuery& Query)
	{
		return Query.ListenerId == ListenerId && Query.TargetId == TargetId;
	});

	if (!Query)
	{
		return -1.0f;
	}

	return Query->GainValue;
}

FVector UAdvancedSightSystem::GetLastKnownLocationFor(const uint32 ListenerId, const uint32 TargetId) const
{
	const FAdvancedSightQuery* Query = Queries.FindByPredicate([ListenerId, TargetId](const FAdvancedSightQuery& Query)
	{
		return Query.ListenerId == ListenerId && Query.TargetId == TargetId;
	});

	if (!Query)
	{
		return FVector::ZeroVector;
	}

	return Query->LastSeenLocation;
}

void UAdvancedSightSystem::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		const UWorld* World = GetWorld();
		const FOnActorSpawned::FDelegate ActorSpawnedDelegate =
			FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleNewActorSpawned);
		NewActorSpawnedDelegateHandle = World->AddOnActorSpawnedHandler(ActorSpawnedDelegate);

		ShouldDebugDraw.AsVariable()->SetOnChangedCallback(
			FConsoleVariableDelegate::CreateUObject(this, &ThisClass::OnDebugDrawStateChanged));
	}
}

void UAdvancedSightSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TRACE_CPUPROFILER_EVENT_SCOPE_STR("UAdvancedSightSystem::Tick");

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FAdvancedSightQuery& Query : Queries)
	{
		UAdvancedSightComponent* SightComponent = Listeners[Query.ListenerId].Get();
		AActor* TargetActor = TargetActors[Query.TargetId].Get();
		bool bIsVisible = false;
		float GainMultiplier = 0.0f;
		if (Query.bIsTargetPerceived)
		{
			bIsVisible = IsVisibleInsideCone(SightComponent, TargetActor, Query.LoseSightRadius, 360.0f);
		}
		else
		{
			for (const FAdvancedSightInfo& SightInfo : Query.SightInfos)
			{
				constexpr float EPSILON = 1.0f;
				const float Radius =
					Query.bWasLastCheckSuccess ? SightInfo.GainRadius + EPSILON : SightInfo.GainRadius;
				const bool bIsVisibleInsideCone =
					IsVisibleInsideCone(SightComponent, TargetActor, Radius, SightInfo.FOV);
				if (bIsVisibleInsideCone)
				{
					bIsVisible = true;
					GainMultiplier = SightInfo.GainMultiplier;
					break;
				}
			}	
		}
		

		if (bIsVisible)
		{
			if (!Query.bWasLastCheckSuccess)
			{
				Query.bWasLastCheckSuccess = true;
				SightComponent->SpotTarget(TargetActor);
			}

			Query.LastSeenLocation = TargetActor->GetActorLocation();
			if (!Query.bIsTargetPerceived)
			{
				Query.GainValue += DeltaTime * GainMultiplier;
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
			if (Query.bWasLastCheckSuccess)
			{
				Query.bWasLastCheckSuccess = false;
				SightComponent->LoseTarget(TargetActor);
			}
			

			if (Query.bIsTargetPerceived)
			{
				Query.LoseSightTimer += DeltaTime;
				if (Query.LoseSightTimer >= Query.LoseSightCooldown)
				{
					Query.bIsTargetPerceived = false;
					SightComponent->ForgetTarget(TargetActor);
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

	if (bShouldDebugDraw && DebugListener.IsValid())
	{
		DrawDebug(DebugListener.Get());
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
	if (!SightData)
	{
		return;
	}
	
	if (SightComponent->GetOwner() == TargetActor)
	{
		return;
	}

	if (SightComponent->GetBodyActor()->Implements<UGenericTeamAgentInterface>()
		&& TargetActor->Implements<UGenericTeamAgentInterface>())
	{
		const auto* ListenerTeamAgent = Cast<IGenericTeamAgentInterface>(SightComponent->GetBodyActor());
		const auto* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetActor);
		const bool bShouldSense = FAISenseAffiliationFilter::ShouldSenseTeam(
			ListenerTeamAgent->GetGenericTeamId(),
			TargetTeamAgent->GetGenericTeamId(),
			SightData->DetectionByAffiliation.GetAsFlags());
		if (!bShouldSense)
		{
			return;
		}
	}

	FAdvancedSightQuery& Query = Queries.AddDefaulted_GetRef();
	Query.ListenerId = SightComponent->GetUniqueID();
	Query.TargetId = TargetActor->GetUniqueID();
	TArray<FAdvancedSightInfo> SortedSightInfos = SightData->SightInfos;
	SortedSightInfos.Sort([](const FAdvancedSightInfo& Lhs, const FAdvancedSightInfo& Rhs)
	{
		return Lhs.GainRadius < Rhs.GainRadius;
	});
	Query.SightInfos = MoveTemp(SortedSightInfos);
	Query.LoseSightRadius = SightData->LoseSightRadius;
	Query.LoseSightCooldown = SightData->LoseSightCooldown;
}

bool UAdvancedSightSystem::IsVisibleInsideCone(
	const UAdvancedSightComponent* SourceComponent,
	const AActor* TargetActor,
	const float Radius,
	const float FOV)
{
	const float CheckRadiusSq = FMath::Square(Radius);
	const FTransform SourceTransform = SourceComponent->GetEyePointOfViewTransform();
	const FVector SourceForward = SourceTransform.GetRotation().Vector();
	TArray<FVector> VisibilityPoints;
	IAdvancedSightTarget::Execute_GetVisibilityPoints(TargetActor, VisibilityPoints);
	for (const FVector& VisibilityPointLocation : VisibilityPoints)
	{
		const float DistanceSq = FVector::DistSquared(SourceTransform.GetLocation(), VisibilityPointLocation);
		if (DistanceSq > CheckRadiusSq)
		{
			continue;
		}

		const FVector DirectionToTarget = (VisibilityPointLocation - SourceTransform.GetLocation()).GetSafeNormal();
		const float DotProduct = FVector::DotProduct(DirectionToTarget, SourceForward);
		const float Angle = FMath::Acos(DotProduct);
		const float MaxAngle = FMath::DegreesToRadians(FOV / 2.0f);
		if (Angle > MaxAngle)
		{
			continue;
		}

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(SourceComponent->GetBodyActor());
		const UWorld* World = TargetActor->GetWorld();
		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			SourceTransform.GetLocation(),
			VisibilityPointLocation,
			ECC_Visibility,
			QueryParams);
		if (bHit && HitResult.GetActor() != TargetActor)
		{
			continue;
		}

		return true;
	}

	return false;
}

void UAdvancedSightSystem::OnDebugDrawStateChanged(IConsoleVariable* ConsoleVariable)
{
	if (ConsoleVariable->GetBool())
	{
		const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (!PC)
		{
			return;
		}

		const APawn* Pawn = PC->GetPawn();
		if (!Pawn)
		{
			return;
		}

		const FVector LocalPawnLocation = Pawn->GetActorLocation();
		float MinDistance = MAX_flt;
		DebugListener = nullptr;
		for (const TTuple<uint32, TWeakObjectPtr<UAdvancedSightComponent>>& Listener : Listeners)
		{
			const AActor* BodyActor = Listener.Value->GetBodyActor();
			const FVector ListenerLocation = BodyActor->GetActorLocation();
			const float DistanceSq = FVector::DistSquared(LocalPawnLocation, ListenerLocation);
			if (DistanceSq < MinDistance)
			{
				MinDistance = DistanceSq;
				DebugListener = Listener.Value.Get();
			}
		}

		if (DebugListener.IsValid())
		{
			bShouldDebugDraw = true;
		}
	}
	else
	{
		bShouldDebugDraw = false;
		DebugListener = nullptr;
	}
}

void UAdvancedSightSystem::DrawDebug(const UAdvancedSightComponent* SightComponent) const
{
	const UAdvancedSightData* SightData = SightComponent->GetSightData();
	if (!SightData)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const FVector CenterLocation = SightComponent->GetBodyActor()->GetActorLocation();
	const FVector ForwardVector = SightComponent->GetBodyActor()->GetActorForwardVector();
	for (const FAdvancedSightInfo& SightInfo : SightData->SightInfos)
	{
		const FVector LeftForward = ForwardVector.RotateAngleAxis(-SightInfo.FOV / 2.0f, FVector::UpVector);
		const FVector RightForward = ForwardVector.RotateAngleAxis(SightInfo.FOV / 2.0f, FVector::UpVector);
		const FVector LeftCenterCorner = CenterLocation + LeftForward * SightInfo.GainRadius;
		const FVector RightCenterCorner = CenterLocation + RightForward * SightInfo.GainRadius;
		DrawDebugLine(World, CenterLocation, LeftCenterCorner, SightInfo.DebugColor);
		DrawDebugLine(World, CenterLocation, RightCenterCorner, SightInfo.DebugColor);
		DrawDebugCircleArc(
			World,
			CenterLocation,
			SightInfo.GainRadius,
			ForwardVector,
			FMath::DegreesToRadians(SightInfo.FOV / 2.0f),
			static_cast<int32>(SightInfo.FOV / 10.0f),
			SightInfo.DebugColor);
	}
	
	const FTransform LoseSightTransform(FQuat(FRotator(90.0, 0.0, 0.0)), CenterLocation);
	DrawDebugCircle(World, LoseSightTransform.ToMatrixNoScale(), SightData->LoseSightRadius, 24, FColor::Black);

	const TArray<AActor*>& PerceivedTargets = SightComponent->GetPerceivedTargets();
	for (const AActor* PerceivedTarget : PerceivedTargets)
	{
		const FVector TargetLocation = PerceivedTarget->GetActorLocation();
		DrawDebugLine(World, CenterLocation, TargetLocation, FColor::Green);
		DrawDebugSphere(World, TargetLocation, 32.0f, 12, FColor::Green);
	}

	const TArray<AActor*>& SpottedTargets = SightComponent->GetSpottedTargets();
	for (const AActor* SpottedTarget : SpottedTargets)
	{
		const FVector TargetLocation = SpottedTarget->GetActorLocation();
		DrawDebugLine(World, CenterLocation, TargetLocation, FColor::Blue);
		DrawDebugSphere(World, TargetLocation, 32.0f, 12, FColor::Blue);
	}

	const TArray<AActor*>& RememberedTargets = SightComponent->GetRememberedTargets();
	const auto* SightSystem = World->GetSubsystem<UAdvancedSightSystem>();
	for (const AActor* RememberedTarget : RememberedTargets)
	{
		const FVector LastKnownLocation =
			SightSystem->GetLastKnownLocationFor(SightComponent->GetUniqueID(), RememberedTarget->GetUniqueID());
		const FVector TargetLocation = RememberedTarget->GetActorLocation();
		DrawDebugLine(World, CenterLocation, LastKnownLocation, FColor::Yellow);
		DrawDebugSphere(World, LastKnownLocation, 32.0f, 12, FColor::Yellow);
		DrawDebugSphere(World, TargetLocation, 32.0f, 12, FColor::Green);
	}
}
