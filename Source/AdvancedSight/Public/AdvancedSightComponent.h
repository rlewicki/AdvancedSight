// Copyright 2024, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AdvancedSightComponent.generated.h"

class UAdvancedSightSystem;
class UAdvancedSightData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAdvancedSightComponentDelegate, AActor*, TargetActor);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ADVANCEDSIGHT_API UAdvancedSightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAdvancedSightComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UAdvancedSightData* GetSightData() const;
	FTransform GetEyePointOfViewTransform() const;
	AActor* GetBodyActor() const;
	bool IsTargetPerceived(const AActor* TargetActor) const;

	UFUNCTION(BlueprintPure)
	float GetGainValueForTarget(const AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable)
	const TArray<AActor*>& GetPerceivedTargets() const;

	UFUNCTION(BlueprintCallable)
	const TArray<AActor*>& GetSpottedTargets() const;

	UFUNCTION(BlueprintCallable)
	const TArray<AActor*>& GetRememberedTargets() const;

	void SpotTarget(AActor* TargetActor);
	void PerceiveTarget(AActor* TargetActor);
	void LoseTarget(AActor* TargetActor);
	void ForgetTarget(AActor* TargetActor);

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetSpotted;

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetPerceived;

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetLost;

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetForgot;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAdvancedSightData> SightData;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> PerceivedTargets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpottedTargets;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> RememberedTargets;

	TWeakObjectPtr<UAdvancedSightSystem> AdvancedSightSystem;
};
