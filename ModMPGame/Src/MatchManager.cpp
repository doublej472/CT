#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

void AMatchManager::execSetPlayerReadyState(FFrame& Stack, void* Result)
{
	P_GET_OBJECT(APlayerController, PC);
	P_GET_UBOOL(IsReady);
	P_FINISH;

	if (!PC)
		return;

	FString PlayerID;

	if (PC->Player)
	{
		if (PC->Player->IsA(UViewport::StaticClass()))
			return;

		PlayerID = GetPlayerID(PC);
	}

	// Get the stats for this player, if there is no change needed just return
	FPlayerStats& Stats = CurrentGamePlayersByID[*PlayerID];
	if (Stats.bIsReady == IsReady)
		return;

	Stats.bIsReady = IsReady;
	if (IsReady)
	{
		nNumReadyPlayers += 1;
		TryFinishReadyCheck();
	}
	else
	{
		nNumReadyPlayers -= 1;
	}
}

void AMatchManager::execGetPlayerReadyState(FFrame& Stack, void* Result)
{
	P_GET_OBJECT(APlayerController, PC);
	P_FINISH;

	FString PlayerID;

	if (PC->Player)
	{
		if (PC->Player->IsA(UViewport::StaticClass()))
			return;

		PlayerID = GetPlayerID(PC);
	}

	// Get the stats for this player, if there is no change needed just return
	FPlayerStats& Stats = CurrentGamePlayersByID[*PlayerID];
	*static_cast<UBOOL*>(Result) = Stats.bIsReady;
}

void AMatchManager::TryFinishReadyCheck()
{
	if (nNumReadyPlayers == GetActivePlayerCount())
	{
		for (TMap<FString, FPlayerStats>::TIterator It(CurrentGamePlayersByID); It; ++It)
		{
			FPlayerStats& Stats = It.Value();
			Stats.bIsReady = false;
		}

		nNumReadyPlayers = 0;
		bReadyCheckActive = false;
		LiveReset();
	}
}