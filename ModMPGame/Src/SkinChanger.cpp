#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

/*
 * SkinChanger
 */

void ASkinChanger::execSetCloneSkin(FFrame& Stack, void* Result) {
	P_GET_OBJECT(AController, C);
	P_GET_INT(SkinIndex);
	P_FINISH;

	if (!C)
		return;

	SkinsByPlayerID[*GetPlayerID(C)].CloneIndex = Clamp(SkinIndex, 0, CloneSkins.Num() - 1);

	NextSkinUpdateTime = 0.0f;
}

void ASkinChanger::execSetTrandoSkin(FFrame& Stack, void* Result) {
	P_GET_OBJECT(AController, C);
	P_GET_INT(SkinIndex);
	P_FINISH;

	if (!C)
		return;

	SkinsByPlayerID[*GetPlayerID(C)].TrandoIndex = Clamp(SkinIndex, 0, TrandoSkins.Num() - 1);

	NextSkinUpdateTime = 0.0f;
}

/*
 * Stupid solution for a stupid issue:
 * Changing a Pawns Skin via RepSkin only shows up on clients if it is actually replicated, i.e. the client can see the Pawn.
 * If a player joins a match and the RepSkin has been set before he joined, it is not visible.
 * Also if the Pawn is out of sight it is despawned and respawned when it gets back. Again, no skin change.
 * That's why the skins are updated periodically by switching between the Shader and the Diffuse texture.
 * This is necessary because the property needs to actually have a different value for the new Skin to be applied on clients.
 * Switching between shader and Diffuse is the best way to do that because visually it is only a short flicker that is only visible when bumpmapping is enabled.
 */
INT ASkinChanger::Tick(FLOAT DeltaTime, ELevelTick TickType) {
	INT Result = Super::Tick(DeltaTime, TickType);

	static bool bUseShaders = true;

	NextSkinUpdateTime -= DeltaTime;

	if (NextSkinUpdateTime <= 0.0f) {
		if (bUseShaders)
			NextSkinUpdateTime = 30.0f;
		else
			NextSkinUpdateTime = 0.1f; // Only show the diffuse for a very short amount of time before switching back to the shader which we actually want

		for (AController* C = Level->ControllerList; C; C = C->nextController) {
			if (!C->Pawn)
				continue;

			FString PlayerID = GetPlayerID(C);
			FSkinEntry* Skin = SkinsByPlayerID.Find(*PlayerID);
			APawn* Pawn = C->Pawn;

			if (Skin) {
				static FName NMPClone("MPClone");
				static FName NMPTrandoshan("MPTrandoshan");

				UBOOL IsClone = Pawn->IsA(NMPClone);
				UBOOL IsTrando = Pawn->IsA(NMPTrandoshan);

				if (!IsClone && !IsTrando)
					continue;

				if (bUseShaders) {
					if (IsClone)
						Pawn->RepSkin = CloneSkins[Skin->CloneIndex];
					else if (IsTrando)
						Pawn->RepSkin = TrandoSkins[Skin->TrandoIndex];

					if (Pawn->Skins.Num() == 0)
						Pawn->Skins.AddItem(Pawn->RepSkin);
					else
						Pawn->Skins[0] = Pawn->RepSkin;
				}
				else {
					if (IsClone)
						Pawn->RepSkin = CloneSkins[Skin->CloneIndex]->Diffuse;
					else if (IsTrando)
						Pawn->RepSkin = TrandoSkins[Skin->TrandoIndex]->Diffuse;
				}
			}
			else if (RandomizeBotSkins && C->IsA<AAIController>()) { // We have an AI controller without a skin, so generate a random one
				FSkinEntry& NewSkin = SkinsByPlayerID[*PlayerID];

				if (NextBotCloneSkin >= CloneSkins.Num())
					NextBotCloneSkin = 0;

				NewSkin.CloneIndex = NextBotCloneSkin++;

				if (NextBotTrandoSkin >= TrandoSkins.Num())
					NextBotTrandoSkin = 0;

				NewSkin.TrandoIndex = NextBotTrandoSkin++;
			}

			C->Pawn->bReplicateMovement = 1;
			C->Pawn->bNetDirty = 1;
		}

		bUseShaders = !bUseShaders;
	}

	// Check if a Pawn was seen by a player that wasn't seen previously and update skins if that is the case
	for (AController* C = Level->ControllerList; C; C = C->nextController) {
		if (!C->Pawn)
			continue;

		FSkinEntry* Skin = SkinsByPlayerID.Find(*GetPlayerID(C));

		if (!Skin)
			continue;

		UBOOL IsPlayer = C->IsA<APlayerController>();
		INT   NumSeenBy = 0;

		for (AController* C2 = Level->ControllerList; C2; C2 = C2->nextController) {
			// We only care if a player was seen by a bot or vice versa. Bots seeing each other does not need to trigger a skin update
			if ((IsPlayer || C2->IsA<APlayerController>()) && C2->LineOfSightTo(C->Pawn))
				++NumSeenBy;
		}

		if (NumSeenBy > Skin->NumSeenBy) {
			NextSkinUpdateTime = 0.1f;
			Skin->NumSeenBy = NumSeenBy;

			break;
		}

		Skin->NumSeenBy = NumSeenBy;
	}

	return Result;
}

void ASkinChanger::Spawned() {
	Super::Spawned();

	// Clear skins every day since the IP of most players has probably changed.
	if (Level->Day != LastSkinResetDay) {
		LastSkinResetDay = Level->Day;
		SkinsByPlayerID.Empty();
	}

	NextBotCloneSkin = static_cast<INT>(appFrand() * CloneSkins.Num());
	NextBotTrandoSkin = static_cast<INT>(appFrand() * TrandoSkins.Num());
}

TMap<FString, ASkinChanger::FSkinEntry> ASkinChanger::SkinsByPlayerID;
INT                                     ASkinChanger::LastSkinResetDay;