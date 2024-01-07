// Copyright 2024, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AdvancedSightData.h"
#include "Subsystems/WorldSubsystem.h"
#include "AdvancedSightSystem.generated.h"

struct FSightSpatialInfo;
class AActor;
class UAdvancedSightComponent;

struct FAdvancedSightQuery
{
	uint32 ListenerId = UINT32_MAX;
	uint32 TargetId = UINT32_MAX;
	FVector LastSeenLocation;
	bool bWasLastCheckSuccess = false;
	bool bIsTargetPerceived = false;
	float LoseSightTimer = 0.0f;
	float LoseSightCooldown = 1.0f;
	float GainValue = 0.0f;
	float LoseSightRadius = -1.0f;
	TArray<FAdvancedSightInfo> SightInfos;
};

UCLASS()
class ADVANCEDSIGHT_API UAdvancedSightSystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	void RegisterListener(UAdvancedSightComponent* SightComponent);
	void UnregisterListener(UAdvancedSightComponent* SightComponent);
	void RegisterTarget(AActor* TargetActor);
	void UnregisterTarget(AActor* TargetActor);
	float GetGainValueForTarget(const uint32 Listener, const uint32 TargetId) const;

	virtual void PostInitProperties() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
protected:
	void HandleNewActorSpawned(AActor* Actor);
	void AddQuery(
		const UAdvancedSightComponent* SightComponent, const AActor* TargetActor, const UAdvancedSightData* SightData);
	static bool IsVisibleInsideCone(
		const UAdvancedSightComponent* SourceComponent,
		const AActor* TargetActor,
		const float Radius,
		const float FOV);

	void OnDebugDrawStateChanged(IConsoleVariable* ConsoleVariable);

	FDelegateHandle NewActorSpawnedDelegateHandle;

	TMap<uint32, TWeakObjectPtr<AActor>> TargetActors;
	TMap<uint32, TWeakObjectPtr<UAdvancedSightComponent>> Listeners;
	TArray<FAdvancedSightQuery> Queries;

	bool bShouldDebugDraw = false;
	TWeakObjectPtr<const UAdvancedSightComponent> DebugListener;
	void DrawDebug(const UAdvancedSightComponent* SightComponent) const;
};
