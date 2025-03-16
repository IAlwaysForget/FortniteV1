#pragma once

#include <stdint.h>

//maybe xorint here, no offset update via IDA :)
#define offs(name, offset) inline uint64_t name = offset

namespace offsets
{
	offs(PersistentLevel, 0x30); //GWorld->PersistentLevel
	offs(WorldSettingsClass, 0x0150); //UEngine->WorldSettingsClass
	offs(WorldGravityZ, 0x0310); //AWorldSettings->WorldGravityZ

	offs(GameState, 0x160);
	offs(PlayerArray, 0x2a8);
	offs(PlayerArraySize, PlayerArray + 0x8);

	offs(OwningGameInstance, 0x1D8);
	offs(LocalPlayers, 0x38);
	offs(PlayerController, 0x30);
	offs(AcknowledgedPawn, 0x338); //APlayerController->AcknowledgedPawn
	offs(RootComponent, 0x198); //AActor->RootComponent (From APawn)
	offs(ComponentVelocity, 0x0168); //USceneComponent->ComponentVelocity
	offs(Id, 0x10); //UObject
	offs(bIsDying, 0x6d4); // AFortPawn->bIsDying 
	offs(bIsDBNO, 0x8F6); // AFortPawn->bIsDBNO

	offs(Mesh, 0x310); //ACharacter->USkeletalMeshComponent			   s
	offs(BoneArray, 0x570); //528 possible or 5F0
	offs(BoneArrayCache, BoneArray + 0x10);
	offs(ComponentToWorld, 0x1C0); //TODO: add IDA info here for ease of updating

	offs(LastSubmitTime, 0x2E0); //in padding  (boundsscale + 0x14)
	offs(LastRenderTimeOnScreen, 0x2F0);

	//we use this struct instead so we can read it in one read request
	struct visible_t
	{
		float LastSubmitTime;
		char pad_0001[0x4];
		float LastRenderTimeOnScreen;
	};

	offs(PlayerState, 0x2b0); //APawn->PlayerState
	offs(TeamIndex, 0x1201); //AFortPlayerStateAthena->TeamIndex;
	offs(PawnPrivate, 0x308); //APlayerState->PawnPrivate
	offs(NameStructure, 0xA98); //in padding
	offs(TargetedFortPawn, 0x1898); // AFortPlayerController->TargetedFortPawn
	offs(bIsTargeting, 0xE58); // CurrentWeapon->bIsTargeting //can also be 0xdb4 + 5  //new one : 0xE58 + 5
	offs(CurrentWeapon, 0x9d8); // AFortPawn->CurrentWeapon
	offs(WeaponData, 0x510); //AFortWeapon->WeaponData
	offs(DisplayName, /*0x90*/0x40); //AFortWeapon->WeaponData->UFortWeaponItemDefinition->DisplayName (Maybe from UFortItemSeriesDefinition)
	offs(CustomTimeDilation, 0x68);
}
