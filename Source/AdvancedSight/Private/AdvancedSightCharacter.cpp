// Copyright 2023, Robert Lewicki, All rights reserved.

#include "AdvancedSightCharacter.h"

#include "AIController.h"

AAdvancedSightCharacter::AAdvancedSightCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AAdvancedSightCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId;
}

FGenericTeamId AAdvancedSightCharacter::GetGenericTeamId() const
{
	return TeamId;
}
