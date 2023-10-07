#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

/*
 * MPBot
 */

int AMPBot::Tick(FLOAT DeltaTime, ELevelTick TickType) {
	/*
	 * This is really stupid but for some reason the movement code
	 * only updates the Pawn's rotation in single player.
	 * The only solution is to pretend we're in SP while calling UpdateMovementAnimation.
	 */
	if (Level->NetMode != NM_Standalone &&
		Pawn &&
		!Pawn->bInterpolating &&
		Pawn->bPhysicsAnimUpdate &&
		Pawn->Mesh) {
		BYTE Nm = Level->NetMode;

		Level->NetMode = NM_Standalone;
		Pawn->UpdateMovementAnimation(DeltaTime);
		Level->NetMode = Nm;
	}

	return Super::Tick(DeltaTime, TickType);
}

void AMPBot::execUpdatePawnAccuracy(FFrame& Stack, void* Result) {
	P_FINISH;

	if (Pawn)
		Pawn->Accuracy = Accuracy;
}
