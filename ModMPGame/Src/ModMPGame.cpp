#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"
#include "../../GameSpyMgr/Inc/GameSpyMgr.h"


/*
 * Shared Functions
 */

FStringTemp GetPlayerID(AController* C) {
	APlayerController* PC = Cast<APlayerController>(C);

	if (PC) {
		// IsA(UNetConnection::StaticClass()) returns false for TcpipConnection - WTF???
		if (PC->Player->IsA<UViewport>()) {
			return "__HOST__";
		}
		else { // Combine ip address with cd key hash to uniquely identify a player even in the same network
			UNetConnection* Con = static_cast<UNetConnection*>(PC->Player);
			FString IP = Con->LowLevelGetRemoteAddress();

			INT Pos = IP.InStr(":", true);

			if (Pos != -1)
				return IP.Left(Pos) + Con->CDKeyHash;

			return IP + Con->CDKeyHash;
		}
	}
	else {
		return FStringTemp("__BOT__") + (C->PlayerReplicationInfo ? *C->PlayerReplicationInfo->PlayerName : "");
	}
}
