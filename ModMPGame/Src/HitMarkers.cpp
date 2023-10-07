#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

TMap<FString, AHitMarkers::FHitEntry> AHitMarkers::LastHitByPlayerID;

void AHitMarkers::Spawned() {
	Super::Spawned();
	GWarn->Logf("Clearing HitMarker data");
	LastHitByPlayerID.Empty();
}

INT AHitMarkers::Tick(FLOAT DeltaTime, ELevelTick TickType) {
	INT Result = Super::Tick(DeltaTime, TickType);
	guard(AHitMarkers::Tick);

	for (AController* C = Level->ControllerList; C; C = C->nextController) {
		if (!C->Pawn)
			continue;
		APawn* Victim = C->Pawn;

		FString VictimID = GetPlayerID(C);
		FHitEntry* LastHitPtr = LastHitByPlayerID.Find(*VictimID);

		guard(hitcheck);
		if (LastHitPtr) {
			FHitEntry LastHit = *LastHitPtr;

			// Hit has been detected!
			if (LastHit.LastHit < Victim->LastHitLocalTime) {
				LastHitPtr->LastHit = Victim->LastHitLocalTime;
				APawn* Attacker = Victim->LastHitBy;
				if (Attacker && Attacker->Controller) {
					FLOAT damage = (LastHitPtr->LastHealth + LastHitPtr->LastShield) - (Victim->Health + Victim->Shields);
					FString AttackerID = GetPlayerID(Attacker->Controller);

					//GWarn->Logf("%s hit %s in the %s for %f",
					//	*Attacker->PlayerReplicationInfo->PlayerName,
					//	*Victim->PlayerReplicationInfo->PlayerName,
					//	*Victim->LastHitBone,
					//	damage);

					guard(message_attacker);
					APlayerController* AttackerPC = Cast<APlayerController>(Attacker->Controller);

					if (AttackerPC) {
						SendHitMessage(AttackerPC, *Victim->PlayerReplicationInfo->PlayerName, *Victim->LastHitBone, damage);
					}
					unguard;
				}
			}
		}
		else {
			FHitEntry& HitEntry = LastHitByPlayerID[*VictimID];
			HitEntry.LastHealth = Victim->Health;
			HitEntry.LastShield = Victim->Shields;
			HitEntry.LastHit = Victim->LastHitLocalTime;
		}
		unguard;

		FHitEntry& HitEntry = LastHitByPlayerID[*VictimID];
		HitEntry.LastHealth = Victim->Health;
		HitEntry.LastShield = Victim->Shields;

		//C->Pawn->bReplicateMovement = 1;
		//C->Pawn->bNetDirty = 1;
	}

	unguard;

	return Result;
}