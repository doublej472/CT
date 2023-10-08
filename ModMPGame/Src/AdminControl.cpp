#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"
#include "../../GameSpyMgr/Inc/GameSpyMgr.h"

static TArray<FString> PreviousGameAdminIDs;
static FOutputDeviceFile AdminControlEventLog;

// Only needed because PlayerController::Player is NULL when GameInfo::Logout is called
static TMap<APlayerController*, FString> PlayerIDsByController;

static struct FAdminControlExec : FExec {
	FExec* OldExec;

	virtual UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar) {
		UBOOL RecognizedCmd = 0;

		if (GAdminControl)
			RecognizedCmd = GAdminControl->ExecCmd(Cmd, NULL);

		if (!RecognizedCmd) {
			if (ParseCommand(&Cmd, "CLEAREVENTLOG")) {
				AdminControlEventLog.Close();
				GFileManager->Delete(AdminControlEventLog.Filename);
				RecognizedCmd = true;
			}
			else {
				RecognizedCmd = OldExec ? OldExec->Exec(Cmd, Ar) : 0;
			}
		}

		return RecognizedCmd;
	}
} GAdminControlExec;

void AAdminControl::Spawned() {
	guard(AAdminControl::Spawned);
	Super::Spawned();

	GAdminControl = this;

	if (GExec != &GAdminControlExec) {
		GAdminControlExec.OldExec = GExec;
		GExec = &GAdminControlExec;
	}

	if (!AdminControlEventLog.Opened) {
		if (AppendEventLog) { // Everything goes into the file that is specified in the ini
			AdminControlEventLog.SetFilename(*EventLogFile);
			AdminControlEventLog.Opened = 1; // Causes content to be appended to log file
		}
		else { // One log file per session
			FFilename Filename = EventLogFile;

			if (Filename.GetExtension() == "")
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

	for (INT i = 0; i < PreviousGameAdminIDs.Num(); ++i)
		CurrentGamePlayersByID[*PreviousGameAdminIDs[i]].bAdmin = 1;

	PlayerIDsByController.Empty();
	unguard;
}

void AAdminControl::Destroy() {
	Super::Destroy();

	if (GAdminControl == this) {
		PreviousGameAdminIDs.Empty();

		for (TMap<FString, FPlayerStats>::TIterator It(CurrentGamePlayersByID); It; ++It) {
			if (It.Value().bAdmin)
				PreviousGameAdminIDs.AddItem(It.Key());
		}

		GAdminControl = NULL;
	}

	if (GExec == &GAdminControlExec) {
		GExec = GAdminControlExec.OldExec;
		GAdminControlExec.OldExec = NULL;
	}
}

void AAdminControl::EventLog(const TCHAR* Msg, FName Event) {
	AdminControlEventLog.Log(static_cast<EName>(Event.GetIndex()), Msg);
	GLog->Log(static_cast<EName>(Event.GetIndex()), Msg);
}

void AAdminControl::execEventLog(FFrame& Stack, void* Result) {
	P_GET_STR(Msg);
	P_GET_NAME(Tag);
	P_FINISH;

	EventLog(*Msg, Tag);
}

void AAdminControl::execSaveStats(FFrame& Stack, void* Result) {
	P_GET_OBJECT(APlayerController, PC);
	P_FINISH;

	if (!PC)
		return;

	FString PlayerID;

	if (PC->Player) {
		// IsA(UNetConnection::StaticClass()) returns false for TcpipConnection - WTF???
		if (PC->Player->IsA(UViewport::StaticClass()))
			return;

		PlayerID = GetPlayerID(PC);
		PlayerIDsByController[PC] = PlayerID;
	}
	else { // PC->Player == NULL happens when a player leaves the server. In that case we need to look up the ID using the controller
		FString* Tmp = PlayerIDsByController.Find(PC);

		if (!Tmp)
			return;

		PlayerID = *Tmp;
		PlayerIDsByController.Remove(PC);
	}

	FPlayerStats& Stats = CurrentGamePlayersByID[*PlayerID];

	PlayerIDsByController[PC] = PlayerID;

	Stats.bAdmin = PC->PlayerReplicationInfo->bAdmin;
	Stats.Kills = PC->PlayerReplicationInfo->Kills;
	Stats.Deaths = PC->PlayerReplicationInfo->Deaths;
	Stats.Score = PC->PlayerReplicationInfo->Score;
	Stats.GoalsScored = PC->PlayerReplicationInfo->GoalsScored;
}

void AAdminControl::execRestoreStats(FFrame& Stack, void* Result) {
	P_GET_OBJECT(APlayerController, PC);
	P_FINISH;

	if (!PC)
		return;

	check(PC->Player);

	if (PC->Player->IsA(UViewport::StaticClass())) {
		PC->PlayerReplicationInfo->bAdmin = 1; // The host is always an admin
		// It doesn't make sense to restore anything else here as the host cannot leave and rejoin without stopping the server
	}
	else {
		FString PlayerID = GetPlayerID(PC);
		const FPlayerStats& Stats = CurrentGamePlayersByID[*PlayerID];

		PlayerIDsByController[PC] = PlayerID;

		PC->PlayerReplicationInfo->bAdmin = Stats.bAdmin;
		PC->PlayerReplicationInfo->Kills = Stats.Kills;
		PC->PlayerReplicationInfo->Deaths = Stats.Deaths;
		PC->PlayerReplicationInfo->Score = Stats.Score;
		PC->PlayerReplicationInfo->GoalsScored = Stats.GoalsScored;
	}
}

void AAdminControl::execReleaseAllCDKeys(FFrame& Stack, void* Result) {
	P_FINISH;
	((GameSpyMgr*)0x1072AF2C)->ReleaseAllCDKey();
}

void AAdminControl::execResetAllStats(FFrame& Stack, void* Result)
{
	P_FINISH;

	for (TMap<FString, FPlayerStats>::TIterator It(CurrentGamePlayersByID); It; ++It)
	{
		It.Value() = FPlayerStats();
	}
}