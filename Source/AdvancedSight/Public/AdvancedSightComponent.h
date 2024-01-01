// Copyright 2023, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AdvancedSightComponent.generated.h"

class UAdvancedSightData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAdvancedSightComponentDelegate, AActor*, TargetActor);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ADVANCEDSIGHT_API UAdvancedSightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAdvancedSightComponent();
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UAdvancedSightData* GetSightData() const;
	FTransform GetEyePointOfViewTransform() const;
	AActor* GetBodyActor() const;
	TSet<TObjectPtr<AActor>> GetPerceivedTargets() const;
	bool IsTargetPerceived(const AActor* TargetActor) const;

	void SpotTarget(AActor* TargetActor);
	void PerceiveTarget(AActor* TargetActor);
	void LoseTarget(AActor* TargetActor);

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetSpotted;

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetPerceived;

	UPROPERTY(BlueprintAssignable)
	FAdvancedSightComponentDelegate OnTargetLost;
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAdvancedSightData> SightData;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> PerceivedTargets;
};
