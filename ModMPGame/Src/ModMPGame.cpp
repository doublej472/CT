#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"
#include "../../GameSpyMgr/Inc/GameSpyMgr.h"

APlayerController* GetLocalPlayerController(){
	TObjectIterator<UViewport> It;

	return It ? It->Actor : NULL;
}

/*
 * AdminControl
 */

static AAdminControl*    GAdminControl = NULL;
static FOutputDeviceFile AdminControlEventLog;

struct FPlayerStats{
	UBOOL bAdmin;
	INT   Kills;
	FLOAT Deaths;
	FLOAT Score;
	INT   GoalsScored;
};

static TArray<FString>                   PreviousGameAdminIDs;
static TMap<FString, FPlayerStats>       CurrentGamePlayersByID;
static TMap<APlayerController*, FString> PlayerIDsByController; // Only needed because PlayerController::Player is NULL when GameInfo::Logout is called


static FStringTemp GetPlayerID(AController* C){
	APlayerController* PC = Cast<APlayerController>(C);

	if(PC){
		// IsA(UNetConnection::StaticClass()) returns false for TcpipConnection - WTF???
		if(PC->Player->IsA<UViewport>()){
			return "__HOST__";
		}else{ // Combine ip address with cd key hash to uniquely identify a player even in the same network
			UNetConnection* Con = static_cast<UNetConnection*>(PC->Player);
			FString IP = Con->LowLevelGetRemoteAddress();

			INT Pos = IP.InStr(":", true);

			if(Pos != -1)
				return IP.Left(Pos) + Con->CDKeyHash;

			return IP + Con->CDKeyHash;
		}
	}else{
		return FStringTemp("__BOT__") + (C->PlayerReplicationInfo ? *C->PlayerReplicationInfo->PlayerName : "");
	}
}

static struct FAdminControlExec : FExec{
	FExec* OldExec;

	virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar){
		UBOOL RecognizedCmd = 0;

		if(GAdminControl)
			RecognizedCmd = GAdminControl->ExecCmd(Cmd, NULL);

		if(!RecognizedCmd){
			if(ParseCommand(&Cmd, "CLEAREVENTLOG")){
				AdminControlEventLog.Close();
				GFileManager->Delete(AdminControlEventLog.Filename);
				RecognizedCmd = true;
			}else{
				RecognizedCmd = OldExec ? OldExec->Exec(Cmd, Ar) : 0;
			}
		}

		return RecognizedCmd;
	}
} GAdminControlExec;

void AAdminControl::Spawned(){
	Super::Spawned();

	GAdminControl = this;

	if(GExec != &GAdminControlExec){
		GAdminControlExec.OldExec = GExec;
		GExec = &GAdminControlExec;
	}

	if(!AdminControlEventLog.Opened){
		if(AppendEventLog){ // Everything goes into the file that is specified in the ini
			AdminControlEventLog.SetFilename(*EventLogFile);
			AdminControlEventLog.Opened = 1; // Causes content to be appended to log file
		}else{ // One log file per session
			FFilename Filename = EventLogFile;

			if(Filename.GetExtension() == "")
				Filename += ".log";

			AdminControlEventLog.SetFilename(*FString::Printf("%s_%i-%i-%i_%i-%i-%i.%s",
			                                                  *Filename.GetBaseFilePath(),
			                                                  Level->Month,
			                                                  Level->Day,
			                                                  Level->Year,
			                                                  Level->Hour,
			                                                  Level->Minute,
			                                                  Level->Second,
			                                                  *Filename.GetExtension()));
		}
	}

	AdminControlEventLog.Unbuffered = 1;
	AdminControlEventLog.Timestamp = EventLogTimestamp;
	AdminControlEventLog.CallLogHook = 0;

	// Restore admins from previous round so they don't have to login again

	CurrentGamePlayersByID.Empty();

	for(INT i = 0; i < PreviousGameAdminIDs.Num(); ++i)
		CurrentGamePlayersByID[*PreviousGameAdminIDs[i]].bAdmin = 1;

	PlayerIDsByController.Empty();
}

void AAdminControl::Destroy(){
	Super::Destroy();

	if(GAdminControl == this){
		PreviousGameAdminIDs.Empty();

		for(TMap<FString, FPlayerStats>::TIterator It(CurrentGamePlayersByID); It; ++It){
			if(It.Value().bAdmin)
				PreviousGameAdminIDs.AddItem(It.Key());
		}

		GAdminControl = NULL;
	}

	if(GExec == &GAdminControlExec){
		GExec = GAdminControlExec.OldExec;
		GAdminControlExec.OldExec = NULL;
	}
}

void AAdminControl::EventLog(const TCHAR* Msg, FName Event){
	AdminControlEventLog.Log(static_cast<EName>(Event.GetIndex()), Msg);
	GLog->Log(static_cast<EName>(Event.GetIndex()), Msg);
}

void AAdminControl::execEventLog(FFrame& Stack, void* Result){
	P_GET_STR(Msg);
	P_GET_NAME(Tag);
	P_FINISH;

	EventLog(*Msg, Tag);
}

void AAdminControl::execSaveStats(FFrame& Stack, void* Result){
	P_GET_OBJECT(APlayerController, PC);
	P_FINISH;

	if(!PC)
		return;

	FString PlayerID;

	if(PC->Player){
		// IsA(UNetConnection::StaticClass()) returns false for TcpipConnection - WTF???
		if(PC->Player->IsA(UViewport::StaticClass()))
			return;

		PlayerID = GetPlayerID(PC);
		PlayerIDsByController[PC] = PlayerID;
	}else{ // PC->Player == NULL happens when a player leaves the server. In that case we need to look up the ID using the controller
		FString* Tmp = PlayerIDsByController.Find(PC);

		if(!Tmp)
			return;

		PlayerID = *Tmp;
		PlayerIDsByController.Remove(PC);
	}

	FPlayerStats& Stats = CurrentGamePlayersByID[*PlayerID];

	PlayerIDsByController[PC] = PlayerID;

	Stats.bAdmin      = PC->PlayerReplicationInfo->bAdmin;
	Stats.Kills       = PC->PlayerReplicationInfo->Kills;
	Stats.Deaths      = PC->PlayerReplicationInfo->Deaths;
	Stats.Score       = PC->PlayerReplicationInfo->Score;
	Stats.GoalsScored = PC->PlayerReplicationInfo->GoalsScored;
}

void AAdminControl::execRestoreStats(FFrame& Stack, void* Result){
	P_GET_OBJECT(APlayerController, PC);
	P_FINISH;

	if(!PC)
		return;

	check(PC->Player);

	if(PC->Player->IsA(UViewport::StaticClass())){
		PC->PlayerReplicationInfo->bAdmin = 1; // The host is always an admin
		// It doesn't make sense to restore anything else here as the host cannot leave and rejoin without stopping the server
	}else{
		FString PlayerID = GetPlayerID(PC);
		const FPlayerStats& Stats = CurrentGamePlayersByID[*PlayerID];

		PlayerIDsByController[PC] = PlayerID;

		PC->PlayerReplicationInfo->bAdmin      = Stats.bAdmin;
		PC->PlayerReplicationInfo->Kills       = Stats.Kills;
		PC->PlayerReplicationInfo->Deaths      = Stats.Deaths;
		PC->PlayerReplicationInfo->Score       = Stats.Score;
		PC->PlayerReplicationInfo->GoalsScored = Stats.GoalsScored;
	}
}

void AAdminControl::execReleaseAllCDKeys(FFrame& Stack, void* Result){
	P_FINISH;
	((GameSpyMgr*)0x1072AF2C)->ReleaseAllCDKey();
}

/*
 * AdminService
 */

void AAdminService::EventLog(const TCHAR* Msg){
	GAdminControl->EventLog(Msg, GetClass()->GetFName());
}

bool AAdminService::CheckCommand(const TCHAR** Stream, const TCHAR* Match){
	if(AdminControl && AdminControl->bPrintCommands){
		AdminControl->CurrentCommands.AddItem(Match);

		return false;
	}

	return ParseCommand(Stream, Match) != 0;
}

void AAdminService::execParseCommand(FFrame& Stack, void* Result){
	P_GET_STR_REF(Stream);
	P_GET_STR(Match);
	P_FINISH;

	const TCHAR* StreamData = **Stream;

	if(CheckCommand(&StreamData, *Match)){
		*Stream = StreamData;
		*static_cast<UBOOL*>(Result) = 1;
	}
}

void AAdminService::execParseToken(FFrame& Stack, void* Result){
	P_GET_STR_REF(Stream);
	P_FINISH;

	const char* StreamData = **Stream;

	*static_cast<FString*>(Result) = ParseToken(StreamData, 0);

	while(*StreamData == ' ' || *StreamData == '\t')
		StreamData++;

	*Stream = StreamData;
}

void AAdminService::execParseIntParam(FFrame& Stack, void* Result){
	P_GET_STR(Stream);
	P_GET_STR(Match);
	P_GET_INT_REF(Value);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = Parse(*Stream, *Match, *Value);
}

void AAdminService::execParseFloatParam(FFrame& Stack, void* Result){
	P_GET_STR(Stream);
	P_GET_STR(Match);
	P_GET_FLOAT_REF(Value);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = Parse(*Stream, *Match, *Value);
}

void AAdminService::execParseStringParam(FFrame& Stack, void* Result){
	P_GET_STR(Stream);
	P_GET_STR(Match);
	P_GET_STR_REF(Value);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = Parse(*Stream, *Match, *Value);
}

void AAdminService::execExecCmd(FFrame& Stack, void* Result){
	P_GET_STR(Cmd);
	P_GET_OBJECT_OPTX(APlayerController, PC, NULL);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = ExecCmd(*Cmd, PC);
}


void AAdminService::execEventLog(FFrame& Stack, void* Result){
	P_GET_STR(Msg);
	P_FINISH;

	EventLog(*Msg);
}

/*
 * BotSupport
 */

static FFilename GetPathFileName(const FString MapName){
	return GFileManager->GetDefaultDirectory() * ".." * "Maps" * "Paths" * FFilename(MapName).GetBaseFilename() + ".ctp";
}

enum ENavPtType{
	NAVPT_PathNode,
	NAVPT_CoverPoint,
	NAVPT_PatrolPoint
};

/*
 * BotSupport
 */

/*
 * FNavPtInfo
 * Information about a navigation point that is written to a file
 */
struct FNavPtInfo{
	ENavPtType Type;
	FVector Location;
	FRotator Rotation;

	friend FArchive& operator<<(FArchive& Ar, FNavPtInfo& N){
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
void ABotSupport::SpawnNavigationPoint(UClass* NavPtClass, const FVector& Location, const FRotator& Rotation){
	guard(ABotSupport::SpawnNavigationPoint);
	check(NavPtClass->IsChildOf(ANavigationPoint::StaticClass()));

	UBOOL IsEd = GIsEditor;

	GIsEditor = 1;

	ANavigationPoint* NavPt = Cast<ANavigationPoint>(XLevel->SpawnActor(NavPtClass, NAME_None, Location, Rotation));

	if(NavPt){
		if(XLevel->Actors.Last() == NavPt && !Level->bStartup){ // We shouldn't mess with the actor list if bStartup == true
			XLevel->Actors.Pop();
			XLevel->Actors.Insert(XLevel->iFirstNetRelevantActor);
			XLevel->Actors[XLevel->iFirstNetRelevantActor] = NavPt;
			++XLevel->iFirstNetRelevantActor;
			++XLevel->iFirstDynamicActor;
		}

		bPathsHaveChanged = 1;
	}else{
		GLog->Logf(NAME_Error, "Failed to spawn %s", *NavPtClass->FriendlyName);
		NavPtFailLocations.AddItem(Location);
	}

	GIsEditor = IsEd;

	unguard;
}

void ABotSupport::ImportPaths(){
	guard(ABotSupport::ImportPaths);

	if(bPathsImported){
		GLog->Log(NAME_Error, "Paths have already been imported");

		return;
	}

	FFilename Filename = GetPathFileName(Level->GetOuter()->GetName());
	FArchive* Ar = GFileManager->CreateFileReader(*Filename);

	if(Ar){
		GLog->Logf("Importing paths from %s", *Filename.GetCleanFilename());

		TArray<FNavPtInfo> NavPtInfo;

		*Ar << NavPtInfo;

		for(int i = 0; i < NavPtInfo.Num(); ++i){
			UClass* NavPtClass;

			if(NavPtInfo[i].Type == NAVPT_CoverPoint)
				NavPtClass = ACoverPoint::StaticClass();
			else if(NavPtInfo[i].Type == NAVPT_PatrolPoint)
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
	}else{
		GLog->Logf(NAME_Error, "Cannot import paths from file '%s'", *Filename.GetCleanFilename());
	}

	if(bPathsImported && bAutoBuildPaths)
		BuildPaths();

	unguard;
}


/*
 * ABotSupport::ExportPaths
 * Exports all navigation points from the current map to a file
 */
void ABotSupport::ExportPaths(){
	guard(ABotSupport::ExportPaths);

	TArray<FNavPtInfo> NavPts;

	foreach(StaticActors, ANavigationPoint, It, XLevel){
		if(It->IsA(APlayerStart::StaticClass()) || It->IsA(AInventorySpot::StaticClass()))
			continue;

		FNavPtInfo NavPtInfo;
		ENavPtType NavPtType;

		if(It->IsA(ACoverPoint::StaticClass()))
			NavPtType = NAVPT_CoverPoint;
		else if(It->IsA(APatrolPoint::StaticClass()))
			NavPtType = NAVPT_PatrolPoint;
		else
			NavPtType = NAVPT_PathNode;

		NavPtInfo.Type = NavPtType;
		NavPtInfo.Location = It->Location;
		NavPtInfo.Rotation = It->Rotation;
		NavPts.AddItem(NavPtInfo);
	}

	if(NavPts.Num() > 0){
		FFilename Filename = GetPathFileName(Level->GetOuter()->GetName());
		GFileManager->MakeDirectory(*Filename.GetPath(), 1);
		FArchive* Ar = GFileManager->CreateFileWriter(*Filename);

		if(Ar){
			GLog->Logf("Exporting paths to %s", *Filename.GetCleanFilename());
			*Ar << NavPts;

			delete Ar;
		}else{
			GLog->Logf(NAME_Error, "Failed to open file '%s' for writing", *Filename.GetCleanFilename());
		}
	}else{
		GLog->Log("Map does not contain any path nodes");
	}

	unguard;
}

/*
 * ABotSupport::BuildPaths
 * Does the same as the build paths option in the editor
 */
void ABotSupport::BuildPaths(){
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
void ABotSupport::ClearPaths(){
	guard(ABotSupport::ClearPaths);
	GPathBuilder.undefinePaths(XLevel);
	unguard;
}

void ABotSupport::Spawned(){
	guard(ABotSupport::Spawned);

	if(!GIsEditor){
		DrawType = DT_None;  // Don't draw the Actor sprite during gameplay

		// Spawn inventory spots for each pickup in the level
		foreach(DynamicActors, APickup, It, XLevel){
			SpawnNavigationPoint(AInventorySpot::StaticClass(),
			                     It->Location + FVector(0, 0, GetDefault<AScout>()->CollisionHeight));
		}

		if(bAutoImportPaths){
			ImportPaths();

			if(bPathsImported && !bAutoBuildPaths) // Paths imported at startup are always built
				BuildPaths();
		}
	}

	unguard;
}

UBOOL ABotSupport::Tick(FLOAT DeltaTime, ELevelTick TickType){
	guard(ABotSupport::Tick);

	bHidden = !bShowPaths;

	/*
	 * Keeping the BotSupport Actor in the players view at all times so that it is always rendered
	 * which is needed when the ShowPaths command was used
	 */
	if(!bHidden){
		APlayerController* Player = GetLocalPlayerController();

		if(Player){
			FVector Loc;

			if(Player->Pawn){
				Loc = Player->Pawn->Location;
				Loc.Z += Player->Pawn->EyeHeight;
			}else{
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
void ABotSupport::PostRender(class FLevelSceneNode* SceneNode, class FRenderInterface* RI){
	guard(ABotSupport::PostRender);
	Super::PostRender(SceneNode, RI);

	FLineBatcher LineBatcher(RI);
	APlayerController* Player = GetLocalPlayerController();
	check(Player);

	// Drawing the collision cylinder for each bot
	// Not terribly useful but can make it easier to see them when testing
	for(int i = 0; i < Bots.Num(); ++i){
		if(Bots[i]->Pawn){
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
	for(int i = 0; i < NavPtFailLocations.Num(); ++i)
		LineBatcher.DrawBox(FBox(NavPtFailLocations[i] - BoxSize, NavPtFailLocations[i] + BoxSize), FColor(255, 0, 0));

	// All navigation points in the level are drawn as a colored box
	foreach(StaticActors, ANavigationPoint, It, XLevel){
		if(It->bDeleteMe)
			continue;

		FColor Color;

		if(It->IsA(APlayerStart::StaticClass()))
			Color = FColor(100, 100, 100);
		else if(It->IsA(ACoverPoint::StaticClass()))
			Color = FColor(0, 0, 255);
		else if(It->IsA(APatrolPoint::StaticClass()))
			Color = FColor(0, 255, 255);
		else
			Color = FColor(150, 100, 150);

		LineBatcher.DrawBox(FBox(It->Location - BoxSize, It->Location + BoxSize), Color);

		if(It->bDirectional)
			LineBatcher.DrawDirectionalArrow(It->Location, It->Rotation, FColor(255, 0, 0));
	}

	// Drawing connections between path nodes like in UnrealEd
	for(ANavigationPoint* Nav = Level->NavigationPointList; Nav; Nav = Nav->nextNavigationPoint){
		for(int i = 0; i < Nav->PathList.Num(); ++i){
			UReachSpec* ReachSpec = Nav->PathList[i];

			if(ReachSpec->Start && ReachSpec->End){
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

bool ABotSupport::ExecCmd(const char* Cmd, class APlayerController* PC){
	// Commands for importing, exporting or building paths

	if(CheckCommand(&Cmd, "IMPORTPATHS")){
		ImportPaths();

		return true;
	}else if(CheckCommand(&Cmd, "EXPORTPATHS")){
		ExportPaths();

		return true;
	}else if(CheckCommand(&Cmd, "BUILDPATHS")){
		BuildPaths();

		return true;
	}else if(CheckCommand(&Cmd, "CLEARPATHS")){
		ClearPaths();

		return true;
	}

	// Commands only available for players and not from the server console

	if(!PC) // Command was entered into server console, so we look if a local player exists and use their controller
		PC = GetLocalPlayerController();

	if(!PC)
		return false;

	// Commands for removing navigation points

	if(CheckCommand(&Cmd, "REMOVENAVIGATIONPOINT")){
		UBOOL IsEditor = GIsEditor;

		GIsEditor = 1;

		foreach(StaticActors, ANavigationPoint, It, XLevel){
			if(!It->IsA(APlayerStart::StaticClass()) &&
			   ((PC->Pawn ? PC->Pawn->Location : PC->Location) - It->Location).SizeSquared() <= 40 * 40){
				XLevel->DestroyActor(*It);
				BuildPaths();
				bPathsHaveChanged = 1;

				break;
			}
		}

		GIsEditor = IsEditor;


		return true;
	}else if(CheckCommand(&Cmd, "REMOVEALLNAVIGATIONPOINTS")){
		UBOOL IsEditor = GIsEditor;

		GIsEditor = 1;

		foreach(StaticActors, ANavigationPoint, It, XLevel){
			if(It->IsA(ANavigationPoint::StaticClass()) && !It->IsA(APlayerStart::StaticClass()))
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

	if(CheckCommand(&Cmd, "PUTPATHNODE"))
		PutNavPtClass = APathNode::StaticClass();
	else if(CheckCommand(&Cmd, "PUTCOVERPOINT"))
		PutNavPtClass = ACoverPoint::StaticClass();
	else if(CheckCommand(&Cmd, "PUTPATROLPOINT"))
		PutNavPtClass = APatrolPoint::StaticClass();

	if(PutNavPtClass){
		FVector Loc;
		FRotator Rot(0, 0, 0);

		if(PC->Pawn){
			Loc = PC->Pawn->Location;
			Rot.Yaw = PC->Pawn->Rotation.Yaw;
		}else{
			Loc = PC->Location;
			Rot.Yaw = PC->Rotation.Yaw;
		}

		SpawnNavigationPoint(PutNavPtClass, Loc, Rot);

		if(bAutoBuildPaths)
			BuildPaths();

		return true;
	}

	// Commands only available for the host of a non-dedicated server

	if(GIsClient){
		if(CheckCommand(&Cmd, "SHOWPATHS")){
			bShowPaths = 1;

			return true;
		}else if(CheckCommand(&Cmd, "HIDEPATHS")){
			bShowPaths = 0;

			return true;
		}
	}

	return false;
}

struct FBotInfo{
	FString DisplayName;
	INT     ChosenSkin;
};

static TArray<FBotInfo> BotInfo;

void ABotSupport::execStoreBotInfo(FFrame& Stack, void* Result){
	P_GET_OBJECT(AMPBot, Bot);
	P_FINISH;

	if(!Bot || !Bot->PlayerReplicationInfo)
		return;

	FBotInfo Info;

	Info.DisplayName = Bot->PlayerReplicationInfo->PlayerName;
	Info.ChosenSkin  = Bot->ChosenSkin;

	BotInfo.AddItem(Info);
}

void ABotSupport::execGetBotInfo(FFrame& Stack, void* Result){
	P_GET_STR_REF(DisplayName);
	P_GET_INT_REF(ChosenSkin);
	P_FINISH;

	if(Level->Game->NumBots < BotInfo.Num()){
		*DisplayName = BotInfo[Level->Game->NumBots].DisplayName;
		*ChosenSkin = BotInfo[Level->Game->NumBots].ChosenSkin;
		*static_cast<UBOOL*>(Result) = 1;
	}else{
		*static_cast<UBOOL*>(Result) = 0;
	}
}

/*
 * SkinChanger
 */

void ASkinChanger::execSetCloneSkin(FFrame& Stack, void* Result){
	P_GET_OBJECT(AController, C);
	P_GET_INT(SkinIndex);
	P_FINISH;

	if(!C)
		return;

	SkinsByPlayerID[*GetPlayerID(C)].CloneIndex = Clamp(SkinIndex, 0, CloneSkins.Num() - 1);

	NextSkinUpdateTime = 0.0f;
}

void ASkinChanger::execSetTrandoSkin(FFrame& Stack, void* Result){
	P_GET_OBJECT(AController, C);
	P_GET_INT(SkinIndex);
	P_FINISH;

	if(!C)
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
INT ASkinChanger::Tick(FLOAT DeltaTime, ELevelTick TickType){
	INT Result = Super::Tick(DeltaTime, TickType);

	static bool bUseShaders = true;

	NextSkinUpdateTime -= DeltaTime;

	if(NextSkinUpdateTime <= 0.0f){
		if(bUseShaders)
			NextSkinUpdateTime = 30.0f;
		else
			NextSkinUpdateTime = 0.1f; // Only show the diffuse for a very short amount of time before switching back to the shader which we actually want

		for(AController* C = Level->ControllerList; C; C = C->nextController){
			if(!C->Pawn)
				continue;

			FString PlayerID = GetPlayerID(C);
			FSkinEntry* Skin = SkinsByPlayerID.Find(*PlayerID);
			APawn* Pawn = C->Pawn;

			if(Skin){
				static FName NMPClone("MPClone");
				static FName NMPTrandoshan("MPTrandoshan");

				UBOOL IsClone = Pawn->IsA(NMPClone);
				UBOOL IsTrando = Pawn->IsA(NMPTrandoshan);

				if(!IsClone && !IsTrando)
					continue;

				if(bUseShaders){
					if(IsClone)
						Pawn->RepSkin = CloneSkins[Skin->CloneIndex];
					else if(IsTrando)
						Pawn->RepSkin = TrandoSkins[Skin->TrandoIndex];

					if(Pawn->Skins.Num() == 0)
						Pawn->Skins.AddItem(Pawn->RepSkin);
					else
						Pawn->Skins[0] = Pawn->RepSkin;
				}else{
					if(IsClone)
						Pawn->RepSkin = CloneSkins[Skin->CloneIndex]->Diffuse;
					else if(IsTrando)
						Pawn->RepSkin = TrandoSkins[Skin->TrandoIndex]->Diffuse;
				}
			}else if(RandomizeBotSkins && C->IsA<AAIController>()){ // We have an AI controller without a skin, so generate a random one
				FSkinEntry& NewSkin = SkinsByPlayerID[*PlayerID];

				if(NextBotCloneSkin >= CloneSkins.Num())
					NextBotCloneSkin = 0;

				NewSkin.CloneIndex = NextBotCloneSkin++;

				if(NextBotTrandoSkin >= TrandoSkins.Num())
					NextBotTrandoSkin = 0;

				NewSkin.TrandoIndex = NextBotTrandoSkin++;
			}

			C->Pawn->bReplicateMovement = 1;
			C->Pawn->bNetDirty = 1;
		}

		bUseShaders = !bUseShaders;
	}

	// Check if a Pawn was seen by a player that wasn't seen previously and update skins if that is the case
	for(AController* C = Level->ControllerList; C; C = C->nextController){
		if(!C->Pawn)
			continue;

		FSkinEntry* Skin = SkinsByPlayerID.Find(*GetPlayerID(C));

		if(!Skin)
			continue;

		UBOOL IsPlayer = C->IsA<APlayerController>();
		INT   NumSeenBy = 0;

		for(AController* C2 = Level->ControllerList; C2; C2 = C2->nextController){
			// We only care if a player was seen by a bot or vice versa. Bots seeing each other does not need to trigger a skin update
			if((IsPlayer || C2->IsA<APlayerController>()) && C2->LineOfSightTo(C->Pawn))
				++NumSeenBy;
		}

		if(NumSeenBy > Skin->NumSeenBy){
			NextSkinUpdateTime = 0.1f;
			Skin->NumSeenBy = NumSeenBy;

			break;
		}

		Skin->NumSeenBy = NumSeenBy;
	}

	return Result;
}

void ASkinChanger::Spawned(){
	Super::Spawned();

	// Clear skins every day since the IP of most players has probably changed.
	if(Level->Day != LastSkinResetDay){
		LastSkinResetDay = Level->Day;
		SkinsByPlayerID.Empty();
	}

	NextBotCloneSkin = static_cast<INT>(appFrand() * CloneSkins.Num());
	NextBotTrandoSkin = static_cast<INT>(appFrand() * TrandoSkins.Num());
}

TMap<FString, ASkinChanger::FSkinEntry> ASkinChanger::SkinsByPlayerID;
INT                                     ASkinChanger::LastSkinResetDay;

/*
 * MPBot
 */

int AMPBot::Tick(FLOAT DeltaTime, ELevelTick TickType){
	/*
	 * This is really stupid but for some reason the movement code
	 * only updates the Pawn's rotation in single player.
	 * The only solution is to pretend we're in SP while calling UpdateMovementAnimation.
	 */
	if(Level->NetMode != NM_Standalone &&
	   Pawn &&
	   !Pawn->bInterpolating &&
	   Pawn->bPhysicsAnimUpdate &&
	   Pawn->Mesh){
		BYTE Nm = Level->NetMode;

		Level->NetMode = NM_Standalone;
		Pawn->UpdateMovementAnimation(DeltaTime);
		Level->NetMode = Nm;
	}

	return Super::Tick(DeltaTime, TickType);
}

void AMPBot::execUpdatePawnAccuracy(FFrame& Stack, void* Result){
	P_FINISH;

	if(Pawn)
		Pawn->Accuracy = Accuracy;
}

/*
 * ExportPathsCommandlet
 */
class UExportPathsCommandlet : public UCommandlet{
	DECLARE_CLASS(UExportPathsCommandlet, UCommandlet, 0, ModMPGame);

	void StaticConstructor(){
		LogToStdout = 1;
		IsServer = 1;
		IsClient = 1;
		IsEditor = 1;
		LazyLoad = 1;
		ShowErrorCount = 0;
		ShowBanner = 0;
	}

	virtual INT Main(const TCHAR* Parms){
		FString MapName;

		if(Parse(Parms, "map=", MapName)){
			UPackage* Package = UObject::LoadPackage(NULL, *MapName, LOAD_NoFail);
			ULevel* Level = NULL;

			foreachobj(ULevel, It){
				if(It->IsIn(Package)){
					Level = *It;

					break;
				}
			}

			if(Level){
				ABotSupport* BotSupport = Cast<ABotSupport>(Level->SpawnActor(ABotSupport::StaticClass()));

				if(BotSupport)
					BotSupport->ExportPaths();
				else
					GWarn->Log(NAME_Error, "Unable to export paths");
			}else{
				GWarn->Logf(NAME_Error, "Package '%s' is not a map", *MapName);
			}
		}else{
			GWarn->Log(NAME_Error, "Map to export paths from must be specified with 'map=<MapName>'");
		}

		return 0;
	}
};

IMPLEMENT_CLASS(UExportPathsCommandlet);
