// Copyright 2023, Robert Lewicki, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "AdvancedSightCharacter.generated.h"

UCLASS()
class ADVANCEDSIGHT_API AAdvancedSightCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
public:
	AAdvancedSightCharacter();

	// IGenericTeamAgentInterface begin
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	// IGenericTeamAgentInterface end
protected:
	UPROPERTY(EditDefaultsOnly)
	FGenericTeamId TeamId;
};
