#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

static FFilename GetPathFileName(const FString MapName) {
	return GFileManager->GetDefaultDirectory() * ".." * "Maps" * "Paths" * FFilename(MapName).GetBaseFilename() + ".ctp";
}

enum ENavPtType {
	NAVPT_PathNode,
	NAVPT_CoverPoint,
	NAVPT_PatrolPoint
};

MODMPGAME_API APlayerController* GetLocalPlayerController() {
	TObjectIterator<UViewport> It;

	return It ? It->Actor : NULL;
}

 /*
  * FNavPtInfo
  * Information about a navigation point that is written to a file
  */
struct FNavPtInfo {
	ENavPtType Type;
	FVector Location;
	FRotator Rotation;

	friend FArchive& operator<<(FArchive& Ar, FNavPtInfo& N) {
		int Type = N.Type;

		Ar << Type << N.Location << N.Rotation;
		N.Type = static_cast<ENavPtType>(Type);

		return Ar;
	}
};

/*
 * ABotSupport::SpawnNavigationPoint
 * Spawns a navigation point at the specified position. Can be called during gameplay
 */
void ABotSupport::SpawnNavigationPoint(UClass* NavPtClass, const FVector& Location, const FRotator& Rotation) {
	guard(ABotSupport::SpawnNavigationPoint);
	check(NavPtClass->IsChildOf(ANavigationPoint::StaticClass()));

	UBOOL IsEd = GIsEditor;

	GIsEditor = 1;

	ANavigationPoint* NavPt = Cast<ANavigationPoint>(XLevel->SpawnActor(NavPtClass, NAME_None, Location, Rotation));

	if (NavPt) {
		if (XLevel->Actors.Last() == NavPt && !Level->bStartup) { // We shouldn't mess with the actor list if bStartup == true
			XLevel->Actors.Pop();
			XLevel->Actors.Insert(XLevel->iFirstNetRelevantActor);
			XLevel->Actors[XLevel->iFirstNetRelevantActor] = NavPt;
			++XLevel->iFirstNetRelevantActor;
			++XLevel->iFirstDynamicActor;
		}

		bPathsHaveChanged = 1;
	}
	else {
		GLog->Logf(NAME_Error, "Failed to spawn %s", *NavPtClass->FriendlyName);
		NavPtFailLocations.AddItem(Location);
	}

	GIsEditor = IsEd;

	unguard;
}

void ABotSupport::ImportPaths() {
	guard(ABotSupport::ImportPaths);

	if (bPathsImported) {
		GLog->Log(NAME_Error, "Paths have already been imported");

		return;
	}

	FFilename Filename = GetPathFileName(Level->GetOuter()->GetName());
	FArchive* Ar = GFileManager->CreateFileReader(*Filename);

	if (Ar) {
		GLog->Logf("Importing paths from %s", *Filename.GetCleanFilename());

		TArray<FNavPtInfo> NavPtInfo;

		*Ar << NavPtInfo;

		for (int i = 0; i < NavPtInfo.Num(); ++i) {
			UClass* NavPtClass;

			if (NavPtInfo[i].Type == NAVPT_CoverPoint)
				NavPtClass = ACoverPoint::StaticClass();
			else if (NavPtInfo[i].Type == NAVPT_PatrolPoint)
				NavPtClass = APatrolPoint::StaticClass();
			else
				NavPtClass = APathNode::StaticClass();

			SpawnNavigationPoint(
				NavPtClass,
				NavPtInfo[i].Location,
				NavPtInfo[i].Rotation
			);
		}

		bPathsImported = 1;

		delete Ar;
	}
	else {
		GLog->Logf(NAME_Error, "Cannot import paths from file '%s'", *Filename.GetCleanFilename());
	}

	if (bPathsImported && bAutoBuildPaths)
		BuildPaths();

	unguard;
}

/*
 * ABotSupport::ExportPaths
 * Exports all navigation points from the current map to a file
 */
void ABotSupport::ExportPaths() {
	guard(ABotSupport::ExportPaths);

	TArray<FNavPtInfo> NavPts;

	foreach(StaticActors, ANavigationPoint, It, XLevel) {
		if (It->IsA(APlayerStart::StaticClass()) || It->IsA(AInventorySpot::StaticClass()))
			continue;

		FNavPtInfo NavPtInfo;
		ENavPtType NavPtType;

		if (It->IsA(ACoverPoint::StaticClass()))
			NavPtType = NAVPT_CoverPoint;
		else if (It->IsA(APatrolPoint::StaticClass()))
			NavPtType = NAVPT_PatrolPoint;
		else
			NavPtType = NAVPT_PathNode;

		NavPtInfo.Type = NavPtType;
		NavPtInfo.Location = It->Location;
		NavPtInfo.Rotation = It->Rotation;
		NavPts.AddItem(NavPtInfo);
	}

	if (NavPts.Num() > 0) {
		FFilename Filename = GetPathFileName(Level->GetOuter()->GetName());
		GFileManager->MakeDirectory(*Filename.GetPath(), 1);
		FArchive* Ar = GFileManager->CreateFileWriter(*Filename);

		if (Ar) {
			GLog->Logf("Exporting paths to %s", *Filename.GetCleanFilename());
			*Ar << NavPts;

			delete Ar;
		}
		else {
			GLog->Logf(NAME_Error, "Failed to open file '%s' for writing", *Filename.GetCleanFilename());
		}
	}
	else {
		GLog->Log("Map does not contain any path nodes");
	}

	unguard;
}

/*
 * ABotSupport::BuildPaths
 * Does the same as the build paths option in the editor
 */
void ABotSupport::BuildPaths() {
	guard(ABotSupport::BuildPaths);

	UBOOL IsEd = GIsEditor;
	UBOOL BegunPlay = Level->bBegunPlay;

	Level->bBegunPlay = 0;
	GIsEditor = 1;

	GPathBuilder.definePaths(XLevel);
	bPathsHaveChanged = 1;

	GIsEditor = IsEd;
	Level->bBegunPlay = 1; // Actor script events are only called if Level->bBegunPlay == true which is not the case when paths are loaded at startup

	SetupPatrolRoute();

	Level->bBegunPlay = BegunPlay;

	unguard;
}

/*
 * ABotSupport::ClearPaths
 * Removes all existing paths but keeps navigation points intact
 */
void ABotSupport::ClearPaths() {
	guard(ABotSupport::ClearPaths);
	GPathBuilder.undefinePaths(XLevel);
	unguard;
}

void ABotSupport::Spawned() {
	guard(ABotSupport::Spawned);

	if (!GIsEditor) {
		DrawType = DT_None;  // Don't draw the Actor sprite during gameplay

		// Spawn inventory spots for each pickup in the level
		foreach(DynamicActors, APickup, It, XLevel) {
			SpawnNavigationPoint(AInventorySpot::StaticClass(),
				It->Location + FVector(0, 0, GetDefault<AScout>()->CollisionHeight));
		}

		if (bAutoImportPaths) {
			ImportPaths();

			if (bPathsImported && !bAutoBuildPaths) // Paths imported at startup are always built
				BuildPaths();
		}
	}

	unguard;
}

UBOOL ABotSupport::Tick(FLOAT DeltaTime, ELevelTick TickType) {
	guard(ABotSupport::Tick);

	bHidden = !bShowPaths;

	/*
	 * Keeping the BotSupport Actor in the players view at all times so that it is always rendered
	 * which is needed when the ShowPaths command was used
	 */
	if (!bHidden) {
		APlayerController* Player = GetLocalPlayerController();

		if (Player) {
			FVector Loc;

			if (Player->Pawn) {
				Loc = Player->Pawn->Location;
				Loc.Z += Player->Pawn->EyeHeight;
			}
			else {
				Loc = Player->Location;
			}

			XLevel->FarMoveActor(this, Loc + Player->Rotation.Vector());
		}
	}

	return Super::Tick(DeltaTime, TickType);

	unguard;
}

/*
 * ABotSupport::PostRender
 */
void ABotSupport::PostRender(class FLevelSceneNode* SceneNode, class FRenderInterface* RI) {
	guard(ABotSupport::PostRender);
	Super::PostRender(SceneNode, RI);

	FLineBatcher LineBatcher(RI);
	APlayerController* Player = GetLocalPlayerController();
	check(Player);

	// Drawing the collision cylinder for each bot
	// Not terribly useful but can make it easier to see them when testing
	for (int i = 0; i < Bots.Num(); ++i) {
		if (Bots[i]->Pawn) {
			LineBatcher.DrawCylinder(
				Bots[i]->Pawn->Location,
				FVector(0, 0, 1),
				FColor(255, 0, 255),
				Bots[i]->Pawn->CollisionRadius,
				Bots[i]->Pawn->CollisionHeight,
				16
			);

			Bots[i]->DebugDraw(LineBatcher);
		}
	}

	FVector BoxSize(8, 8, 8);

	// Navigation points that failed to spawn are drawn as a red box for debugging purposes
	for (int i = 0; i < NavPtFailLocations.Num(); ++i)
		LineBatcher.DrawBox(FBox(NavPtFailLocations[i] - BoxSize, NavPtFailLocations[i] + BoxSize), FColor(255, 0, 0));

	// All navigation points in the level are drawn as a colored box
	foreach(StaticActors, ANavigationPoint, It, XLevel) {
		if (It->bDeleteMe)
			continue;

		FColor Color;

		if (It->IsA(APlayerStart::StaticClass()))
			Color = FColor(100, 100, 100);
		else if (It->IsA(ACoverPoint::StaticClass()))
			Color = FColor(0, 0, 255);
		else if (It->IsA(APatrolPoint::StaticClass()))
			Color = FColor(0, 255, 255);
		else
			Color = FColor(150, 100, 150);

		LineBatcher.DrawBox(FBox(It->Location - BoxSize, It->Location + BoxSize), Color);

		if (It->bDirectional)
			LineBatcher.DrawDirectionalArrow(It->Location, It->Rotation, FColor(255, 0, 0));
	}

	// Drawing connections between path nodes like in UnrealEd
	for (ANavigationPoint* Nav = Level->NavigationPointList; Nav; Nav = Nav->nextNavigationPoint) {
		for (int i = 0; i < Nav->PathList.Num(); ++i) {
			UReachSpec* ReachSpec = Nav->PathList[i];

			if (ReachSpec->Start && ReachSpec->End) {
				LineBatcher.DrawLine(
					ReachSpec->Start->Location + FVector(0, 0, 8),
					ReachSpec->End->Location,
					ReachSpec->PathColor()
				);

				// make arrowhead to show L.D direction of path
				FVector Dir = ReachSpec->End->Location - ReachSpec->Start->Location - FVector(0, 0, 8);
				Dir.Normalize();

				LineBatcher.DrawLine(
					ReachSpec->End->Location - 12 * Dir + FVector(0, 0, 3),
					ReachSpec->End->Location - 6 * Dir,
					ReachSpec->PathColor()
				);

				LineBatcher.DrawLine(
					ReachSpec->End->Location - 12 * Dir - FVector(0, 0, 3),
					ReachSpec->End->Location - 6 * Dir,
					ReachSpec->PathColor()
				);

				LineBatcher.DrawLine(
					ReachSpec->End->Location - 12 * Dir + FVector(0, 0, 3),
					ReachSpec->End->Location - 12 * Dir - FVector(0, 0, 3),
					ReachSpec->PathColor()
				);
			}
		}
	}

	unguard;
}

bool ABotSupport::ExecCmd(const char* Cmd, class APlayerController* PC) {
	// Commands for importing, exporting or building paths

	if (CheckCommand(&Cmd, "IMPORTPATHS")) {
		ImportPaths();

		return true;
	}
	else if (CheckCommand(&Cmd, "EXPORTPATHS")) {
		ExportPaths();

		return true;
	}
	else if (CheckCommand(&Cmd, "BUILDPATHS")) {
		BuildPaths();

		return true;
	}
	else if (CheckCommand(&Cmd, "CLEARPATHS")) {
		ClearPaths();

		return true;
	}

	// Commands only available for players and not from the server console

	if (!PC) // Command was entered into server console, so we look if a local player exists and use their controller
		PC = GetLocalPlayerController();

	if (!PC)
		return false;

	// Commands for removing navigation points

	if (CheckCommand(&Cmd, "REMOVENAVIGATIONPOINT")) {
		UBOOL IsEditor = GIsEditor;

		GIsEditor = 1;

		foreach(StaticActors, ANavigationPoint, It, XLevel) {
			if (!It->IsA(APlayerStart::StaticClass()) &&
				((PC->Pawn ? PC->Pawn->Location : PC->Location) - It->Location).SizeSquared() <= 40 * 40) {
				XLevel->DestroyActor(*It);
				BuildPaths();
				bPathsHaveChanged = 1;

				break;
			}
		}

		GIsEditor = IsEditor;


		return true;
	}
	else if (CheckCommand(&Cmd, "REMOVEALLNAVIGATIONPOINTS")) {
		UBOOL IsEditor = GIsEditor;

		GIsEditor = 1;

		foreach(StaticActors, ANavigationPoint, It, XLevel) {
			if (It->IsA(ANavigationPoint::StaticClass()) && !It->IsA(APlayerStart::StaticClass()))
				XLevel->DestroyActor(*It);

			bPathsHaveChanged = 1;
		}

		GIsEditor = IsEditor;
		bPathsImported = 0;

		BuildPaths();

		return true;
	}

	// Commands for spawning navigation points

	UClass* PutNavPtClass = NULL;

	if (CheckCommand(&Cmd, "PUTPATHNODE"))
		PutNavPtClass = APathNode::StaticClass();
	else if (CheckCommand(&Cmd, "PUTCOVERPOINT"))
		PutNavPtClass = ACoverPoint::StaticClass();
	else if (CheckCommand(&Cmd, "PUTPATROLPOINT"))
		PutNavPtClass = APatrolPoint::StaticClass();

	if (PutNavPtClass) {
		FVector Loc;
		FRotator Rot(0, 0, 0);

		if (PC->Pawn) {
			Loc = PC->Pawn->Location;
			Rot.Yaw = PC->Pawn->Rotation.Yaw;
		}
		else {
			Loc = PC->Location;
			Rot.Yaw = PC->Rotation.Yaw;
		}

		SpawnNavigationPoint(PutNavPtClass, Loc, Rot);

		if (bAutoBuildPaths)
			BuildPaths();

		return true;
	}

	// Commands only available for the host of a non-dedicated server

	if (GIsClient) {
		if (CheckCommand(&Cmd, "SHOWPATHS")) {
			bShowPaths = 1;

			return true;
		}
		else if (CheckCommand(&Cmd, "HIDEPATHS")) {
			bShowPaths = 0;

			return true;
		}
	}

	return false;
}

struct FBotInfo {
	FString DisplayName;
	INT     ChosenSkin;
};

static TArray<FBotInfo> BotInfo;

void ABotSupport::execStoreBotInfo(FFrame& Stack, void* Result) {
	P_GET_OBJECT(AMPBot, Bot);
	P_FINISH;

	if (!Bot || !Bot->PlayerReplicationInfo)
		return;

	FBotInfo Info;

	Info.DisplayName = Bot->PlayerReplicationInfo->PlayerName;
	Info.ChosenSkin = Bot->ChosenSkin;

	BotInfo.AddItem(Info);
}

void ABotSupport::execGetBotInfo(FFrame& Stack, void* Result) {
	P_GET_STR_REF(DisplayName);
	P_GET_INT_REF(ChosenSkin);
	P_FINISH;

	if (Level->Game->NumBots < BotInfo.Num()) {
		*DisplayName = BotInfo[Level->Game->NumBots].DisplayName;
		*ChosenSkin = BotInfo[Level->Game->NumBots].ChosenSkin;
		*static_cast<UBOOL*>(Result) = 1;
	}
	else {
		*static_cast<UBOOL*>(Result) = 0;
	}
}