// Copyright 2024, Robert Lewicki, All rights reserved.

#include "AdvancedSightSystem.h"

#include "AdvancedSightComponent.h"
#include "AdvancedSightData.h"
#include "AdvancedSightSettings.h"
#include "AdvancedSightTarget.h"
#include "AdvancedSightTargetComponent.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<bool> CVarShouldDebugDraw(
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

		CVarShouldDebugDraw.AsVariable()->SetOnChangedCallback(
			FConsoleVariableDelegate::CreateUObject(this, &ThisClass::OnDebugDrawStateChanged));
		bShouldDebugDraw = CVarShouldDebugDraw.GetValueOnGameThread();
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

	const ECollisionChannel SightCollisionChannel = GetDefault<UAdvancedSightSettings>()->AdvancedSightCollisionChannel;
	ParallelFor(Queries.Num(), [this, SightCollisionChannel](int32 Index)
	{
		FAdvancedSightQuery& Query = Queries[Index];
		const UAdvancedSightComponent* SightComponent = Listeners[Query.ListenerId].Get();
		const AActor* TargetActor = TargetActors[Query.TargetId].Get();
		Query.bIsCurrentCheckSuccess = false;
		ResetPointsVisibility(Query.bTargetVisibilityPointsFlag);
		if (Query.bIsTargetPerceived)
		{
			Query.bIsCurrentCheckSuccess = IsVisibleInsideCone(
				SightComponent,
				TargetActor,
				Query.LoseSightRadius,
				360.0f,
				SightCollisionChannel,
				Query.bTargetVisibilityPointsFlag);
		}
		else
		{
			for (const FAdvancedSightInfo& SightInfo : Query.SightInfos)
			{
				constexpr float EPSILON = 1.0f;
				const float Radius =
					Query.bWasLastCheckSuccess ? SightInfo.GainRadius + EPSILON : SightInfo.GainRadius;
				const bool bIsVisibleInsideCone = IsVisibleInsideCone(
					SightComponent,
					TargetActor,
					Radius,
					SightInfo.FOV,
					SightCollisionChannel,
					Query.bTargetVisibilityPointsFlag);
				if (bIsVisibleInsideCone)
				{
					Query.bIsCurrentCheckSuccess = true;
					Query.CurrentGainMultiplier = SightInfo.GainMultiplier;
					break;
				}
			}	
		}
	},
	false);

	for (FAdvancedSightQuery& Query : Queries)
	{
		UAdvancedSightComponent* SightComponent = Listeners[Query.ListenerId].Get();
		AActor* TargetActor = TargetActors[Query.TargetId].Get();
		if (Query.bIsCurrentCheckSuccess)
		{
			if (!Query.bWasLastCheckSuccess)
			{
				Query.bWasLastCheckSuccess = true;
				SightComponent->SpotTarget(TargetActor);
			}

			Query.LastSeenLocation = TargetActor->GetActorLocation();
			if (!Query.bIsTargetPerceived)
			{
				Query.GainValue += DeltaTime * Query.CurrentGainMultiplier;
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
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAdvancedSightSystem, STATGROUP_Tickables);
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
	const float FOV,
	ECollisionChannel CollisionChannel,
	int32& VisibilityPointsFlags)
{
	const float CheckRadiusSq = FMath::Square(Radius);
	const FTransform SourceTransform = SourceComponent->GetEyePointOfViewTransform();
	const FVector SourceForward = SourceTransform.GetRotation().Vector();
	TArray<FVector> VisibilityPoints;
	GetVisibilityPointsForActor(TargetActor, VisibilityPoints);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(SourceComponent->GetBodyActor());
	for (int32 Index = 0; Index < VisibilityPoints.Num(); Index++)
	{
		const float DistanceSq = FVector::DistSquared(SourceTransform.GetLocation(), VisibilityPoints[Index]);
		if (DistanceSq > CheckRadiusSq)
		{
			continue;
		}

		const FVector DirectionToTarget = (VisibilityPoints[Index] - SourceTransform.GetLocation()).GetSafeNormal();
		const float DotProduct = FVector::DotProduct(DirectionToTarget, SourceForward);
		const float Angle = FMath::Acos(DotProduct);
		const float MaxAngle = FMath::DegreesToRadians(FOV / 2.0f);
		if (Angle > MaxAngle)
		{
			continue;
		}

		FHitResult HitResult;
		const UWorld* World = TargetActor->GetWorld();
		check(World);
		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			SourceTransform.GetLocation(),
			VisibilityPoints[Index],
			CollisionChannel,
			QueryParams);
		if (bHit && HitResult.GetActor() != TargetActor)
		{
			continue;
		}

		SetPointVisible(VisibilityPointsFlags, Index, true);
		return true;
	}

	return false;
}

void UAdvancedSightSystem::SetPointVisible(int32& Flags, int32 PointIndex, bool bIsVisible)
{
	if (bIsVisible)
	{
		Flags |= (1 << PointIndex);
	}
	else
	{
		Flags &= ~(1 << PointIndex);
	}
}

bool UAdvancedSightSystem::IsPointVisible(int32 Flags, int32 PointIndex)
{
	return (Flags >> PointIndex) & 1;
}

void UAdvancedSightSystem::ResetPointsVisibility(int32& Flags)
{
	Flags = 0;
}

void UAdvancedSightSystem::GetVisibilityPointsForActor(const AActor* Actor, TArray<FVector>& OutVisibilityPoints)
{
	if (const auto* TargetComponent = Actor->FindComponentByClass<UAdvancedSightTargetComponent>())
	{
		TargetComponent->GetVisibilityPoints(OutVisibilityPoints);
	}
	else
	{
		OutVisibilityPoints.Add(Actor->GetActorLocation());
	}
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

	const FDebugDrawInfo& DebugDrawInfo = GetDefault<UAdvancedSightSettings>()->DebugDrawInfo;
	const TArray<AActor*>& PerceivedTargets = SightComponent->GetPerceivedTargets();
	for (const AActor* PerceivedTarget : PerceivedTargets)
	{
		TArray<FVector> VisibilityPoints;
		GetVisibilityPointsForActor(PerceivedTarget, VisibilityPoints);

		const uint32 ListenerId = SightComponent->GetUniqueID();
		const uint32 TargetId = PerceivedTarget->GetUniqueID();
		const FAdvancedSightQuery* Query = Queries.FindByPredicate([ListenerId, TargetId](const FAdvancedSightQuery& Query)
		{
			return Query.ListenerId == ListenerId && Query.TargetId == TargetId;
		});
		if (!ensure(Query))
		{
			continue;
		}

		for (int32 Index = 0; Index < VisibilityPoints.Num(); Index++)
		{
			const bool bIsPointVisible = IsPointVisible(Query->bTargetVisibilityPointsFlag, Index);
			if (bIsPointVisible)
			{
				DrawDebugLine(World, CenterLocation, VisibilityPoints[Index], DebugDrawInfo.PerceivedTarget_VisibleColor);
				DrawDebugSphere(
					World,
					VisibilityPoints[Index],
					DebugDrawInfo.VisibilityPointRadius,
					DebugDrawInfo.VisibilityPointSphereSegments,
					DebugDrawInfo.PerceivedTarget_VisibleColor);
			}
			else
			{
				DrawDebugLine(
					World, CenterLocation, VisibilityPoints[Index], DebugDrawInfo.PerceivedTarget_UnconfirmedColor);
				DrawDebugSphere(
					World,
					VisibilityPoints[Index],
					DebugDrawInfo.VisibilityPointRadius,
					DebugDrawInfo.VisibilityPointSphereSegments,
					DebugDrawInfo.PerceivedTarget_UnconfirmedColor);
			}
		}
	}

	const TArray<AActor*>& SpottedTargets = SightComponent->GetSpottedTargets();
	for (const AActor* SpottedTarget : SpottedTargets)
	{
		TArray<FVector> VisibilityPoints;
		GetVisibilityPointsForActor(SpottedTarget, VisibilityPoints);
		const uint32 ListenerId = SightComponent->GetUniqueID();
		const uint32 TargetId = SpottedTarget->GetUniqueID();
		const FAdvancedSightQuery* Query = Queries.FindByPredicate([ListenerId, TargetId](const FAdvancedSightQuery& Query)
		{
			return Query.ListenerId == ListenerId && Query.TargetId == TargetId;
		});
		if (!ensure(Query))
		{
			continue;
		}
		
		for (int32 Index = 0; Index < VisibilityPoints.Num(); Index++)
		{
			const bool bIsPointVisible = IsPointVisible(Query->bTargetVisibilityPointsFlag, Index);
			if (bIsPointVisible)
			{
				DrawDebugLine(World, CenterLocation, VisibilityPoints[Index], DebugDrawInfo.SpottedTarget_VisibleColor);
				DrawDebugSphere(
					World,
					VisibilityPoints[Index],
					DebugDrawInfo.VisibilityPointRadius,
					DebugDrawInfo.VisibilityPointSphereSegments,
					DebugDrawInfo.SpottedTarget_VisibleColor);		
			}
			else
			{
				DrawDebugLine(World, CenterLocation, VisibilityPoints[Index], DebugDrawInfo.SpottedTarget_UnconfirmedColor);
				DrawDebugSphere(
					World,
					VisibilityPoints[Index],
					DebugDrawInfo.VisibilityPointRadius,
					DebugDrawInfo.VisibilityPointSphereSegments,
					DebugDrawInfo.SpottedTarget_UnconfirmedColor);
			}
		}
	}

	const TArray<AActor*>& RememberedTargets = SightComponent->GetRememberedTargets();
	const auto* SightSystem = World->GetSubsystem<UAdvancedSightSystem>();
	for (const AActor* RememberedTarget : RememberedTargets)
	{
		const FVector LastKnownLocation =
			SightSystem->GetLastKnownLocationFor(SightComponent->GetUniqueID(), RememberedTarget->GetUniqueID());
		DrawDebugLine(World, CenterLocation, LastKnownLocation, FColor::Yellow);
		DrawDebugSphere(
			World,
			LastKnownLocation,
			DebugDrawInfo.VisibilityPointRadius,
			DebugDrawInfo.VisibilityPointSphereSegments,
			DebugDrawInfo.LastKnownLocationColor);
		TArray<FVector> VisibilityPoints;
		GetVisibilityPointsForActor(RememberedTarget, VisibilityPoints);
		for (const FVector& VisibilityPoint : VisibilityPoints)
		{
			DrawDebugSphere(
				World,
				VisibilityPoint,
				DebugDrawInfo.VisibilityPointRadius,
				DebugDrawInfo.VisibilityPointSphereSegments,
				DebugDrawInfo.NotPerceivedColor);
		}
	}
}
