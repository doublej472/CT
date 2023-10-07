#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

void AAdminService::EventLog(const TCHAR* Msg) {
	GAdminControl->EventLog(Msg, GetClass()->GetFName());
}

bool AAdminService::CheckCommand(const TCHAR** Stream, const TCHAR* Match) {
	if (AdminControl && AdminControl->bPrintCommands) {
		AdminControl->CurrentCommands.AddItem(Match);

		return false;
	}

	return ParseCommand(Stream, Match) != 0;
}

void AAdminService::execParseCommand(FFrame& Stack, void* Result) {
	P_GET_STR_REF(Stream);
	P_GET_STR(Match);
	P_FINISH;

	const TCHAR* StreamData = **Stream;

	if (CheckCommand(&StreamData, *Match)) {
		*Stream = StreamData;
		*static_cast<UBOOL*>(Result) = 1;
	}
}

void AAdminService::execParseToken(FFrame& Stack, void* Result) {
	P_GET_STR_REF(Stream);
	P_FINISH;

	const char* StreamData = **Stream;

	*static_cast<FString*>(Result) = ParseToken(StreamData, 0);

	while (*StreamData == ' ' || *StreamData == '\t')
		StreamData++;

	*Stream = StreamData;
}

void AAdminService::execParseIntParam(FFrame& Stack, void* Result) {
	P_GET_STR(Stream);
	P_GET_STR(Match);
	P_GET_INT_REF(Value);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = Parse(*Stream, *Match, *Value);
}

void AAdminService::execParseFloatParam(FFrame& Stack, void* Result) {
	P_GET_STR(Stream);
	P_GET_STR(Match);
	P_GET_FLOAT_REF(Value);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = Parse(*Stream, *Match, *Value);
}

void AAdminService::execParseStringParam(FFrame& Stack, void* Result) {
	P_GET_STR(Stream);
	P_GET_STR(Match);
	P_GET_STR_REF(Value);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = Parse(*Stream, *Match, *Value);
}

void AAdminService::execExecCmd(FFrame& Stack, void* Result) {
	P_GET_STR(Cmd);
	P_GET_OBJECT_OPTX(APlayerController, PC, NULL);
	P_FINISH;

	*static_cast<UBOOL*>(Result) = ExecCmd(*Cmd, PC);
}


void AAdminService::execEventLog(FFrame& Stack, void* Result) {
	P_GET_STR(Msg);
	P_FINISH;

	EventLog(*Msg);
}
