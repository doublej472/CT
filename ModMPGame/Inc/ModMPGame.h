#include "../../Engine/Inc/Engine.h"
#include "../../CTGame/Inc/CTGame.h"

#ifndef MODMPGAME_API
#define MODMPGAME_API DLL_IMPORT
LINK_LIB(ModMPGame)
#endif

#include "ModMPGameClasses.h"

struct FPlayerStats {
	UBOOL bAdmin;
	INT   Kills;
	FLOAT Deaths;
	FLOAT Score;
	INT   GoalsScored;
	UBOOL bIsReady;
};

static TMap<FString, FPlayerStats> CurrentGamePlayersByID;
static AAdminControl* GAdminControl = NULL;

FStringTemp GetPlayerID(AController* C);