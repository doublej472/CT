#include <stdio.h>

#include "../../Engine/Inc/Engine.h"
#include "HttpServer.h"

// Variables for ServerCommandlet

static const TCHAR* CurrentConsoleCommand;
static RepcomStatsServer srv;

/*
 * Allows user input in the console while running a server.
 * This function runs in a separate thread in order to not having
 * to pause the main loop while waiting for input.
 */
static DWORD WINAPI UpdateServerConsoleInput(PVOID){
	printf("Started UpdateServerConsoleInput Thread\n");
	HANDLE InputHandle = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if(!InputHandle)
		return 1;

	while(GIsRunning && !GIsRequestingExit){
		TCHAR ConsoleCommandBuffer[512];

		if(!CurrentConsoleCommand){
			DWORD InputLen;
			ReadConsoleA(InputHandle, ConsoleCommandBuffer, ARRAY_COUNT(ConsoleCommandBuffer) - 1, &InputLen, NULL);

			if(InputLen > 0){
				TCHAR* Cmd = ConsoleCommandBuffer;

				// Trim spaces from command

				while(InputLen > 0 && appIsSpace(Cmd[InputLen - 1]))
					--InputLen;

				Cmd[InputLen] = '\0';

				while(InputLen > 0 && appIsSpace(*Cmd)){
					++Cmd;
					--InputLen;
				}

				if(InputLen > 0)
					CurrentConsoleCommand = Cmd;
			}
		}else{
			Sleep(100);
		}
	}

	CloseHandle(InputHandle);

	printf("Stopped UpdateServerConsoleInput Thread\n");
	return 0;
}

static DWORD WINAPI ServerStatsServer(PVOID) {
	printf("Started ServerStatsServer Thread\n");
	
	srv.Listen();

	printf("Stopped ServerStatsServer Thread\n");
	return 0;
}

// Replacement for UServerCommandlet::Main since the one from Engine.dll crashes because it doesn't assign a value to GEngine
INT UServerCommandletMain(){
	FString Language;

	if(GConfig->GetFString("Engine.Engine", "Language", Language, "System.ini"))
		UObject::SetLanguage(*Language);

	UClass* EngineClass = LoadClass<UEngine>(NULL, "ini:Engine.Engine.GameEngine", NULL, LOAD_NoFail, NULL);

	GEngine = ConstructObject<UEngine>(EngineClass);
	GEngine->Init();

	// Create input thread
	HANDLE InputThread = CreateThread(NULL, 0, UpdateServerConsoleInput, NULL, 0, NULL);
	HANDLE ServerStatsThread = CreateThread(NULL, 0, ServerStatsServer, NULL, 0, NULL);

	INT TickCount = 0;

	GIsRunning = 1;

	// Use QPC for tracking time
	LARGE_INTEGER OldTime, SecondStartTime;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&OldTime);
	SecondStartTime.QuadPart = OldTime.QuadPart;

	// Enforce optional maximum tick rate
	FLOAT MaxTickRate = GEngine->GetMaxTickRate();

	// Main loop
	while(GIsRunning && !GIsRequestingExit){
		LARGE_INTEGER NewTime;
		QueryPerformanceCounter(&NewTime);

		if(CurrentConsoleCommand){
			if(appStricmp(CurrentConsoleCommand, "CLS") == 0) // In case user wants to clear screen. Can be useful for testing.
				system("cls"); // Hate using system but it's ok here
			else if(appStricmp(CurrentConsoleCommand, "GETMAXTICKRATE") == 0)
				GLog->Logf("%f", MaxTickRate);
			else if (appStrnicmp(CurrentConsoleCommand, "SETMAXTICKRATE", 14) == 0) {
				INT Strlen = appStrlen(CurrentConsoleCommand);
				if (Strlen > 15) {
					FLOAT NewTickRate = appAtof(CurrentConsoleCommand+15);
					if (NewTickRate > 1000.0f || NewTickRate < 0.0f) {
						GWarn->Log("Tickrate must be in range 0.0 -> 1000.0");
					} else {
						MaxTickRate = NewTickRate;
						GLog->Logf("Max tickrate changed to %f", MaxTickRate);
					}
				}
			}
			else if(!GEngine->Exec(CurrentConsoleCommand, *GWarn))
				GWarn->Log(LocalizeError("Exec", "Core"));

			CurrentConsoleCommand = NULL;
		}

		LARGE_INTEGER ElapsedTime;
		ElapsedTime.QuadPart = NewTime.QuadPart - OldTime.QuadPart;

		// Calculate the deltatime in milliseconds
		DOUBLE tickDelta = ((DOUBLE) ElapsedTime.QuadPart / (DOUBLE) Frequency.QuadPart);

		// Update the world
		GEngine->Tick(tickDelta);

		//GLog->Logf("delta: %f", tickDelta);

		OldTime.QuadPart = NewTime.QuadPart;
		++TickCount;

		// If it has been 1 second since the last update
		if(OldTime.QuadPart > SecondStartTime.QuadPart + Frequency.QuadPart){
			float UCCTickRate = TickCount / ((DOUBLE) (OldTime.QuadPart - SecondStartTime.QuadPart) / (DOUBLE) Frequency.QuadPart);
			SecondStartTime.QuadPart = OldTime.QuadPart;
			TickCount = 0;
			//GLog->Logf("Tickrate: %f", GEngine->CurrentTickRate);
			struct RepcomStats s = {UCCTickRate, GEngine->CurrentTickRate };
			srv.UpdateStats(s);
		}

		if(MaxTickRate > 0.0f){
			LARGE_INTEGER TickTime;
			LARGE_INTEGER WaitTime;
			LARGE_INTEGER CurrentTime;
			LARGE_INTEGER TargetTime;
			QueryPerformanceCounter(&CurrentTime);

			TickTime.QuadPart = Frequency.QuadPart / MaxTickRate;
			TargetTime.QuadPart = NewTime.QuadPart + TickTime.QuadPart;
			WaitTime.QuadPart = TargetTime.QuadPart - CurrentTime.QuadPart;

			// If we are waiting for over 1 milliseconds
			// Try sleeping, leaving 1 millisecond for busy loop
			DOUBLE WaitTimeDouble = (DOUBLE) WaitTime.QuadPart / (DOUBLE) Frequency.QuadPart;
			if (WaitTimeDouble > 0.001) {
				//GLog->Logf("Sleeping for %f seconds", WaitTimeDouble);
				appSleep(WaitTimeDouble - 0.001);
			}

			// Busy loop until next tick needs to be processed, more accurate
			while (true) {
				LARGE_INTEGER CurrentTime;
				QueryPerformanceCounter(&CurrentTime);

				if (CurrentTime.QuadPart >= TargetTime.QuadPart) {
					break;
				}
			}
		}
	}

	GIsRunning = 0;

	WaitForSingleObject(InputThread, INFINITE);
	WaitForSingleObject(ServerStatsThread, INFINITE);
	CloseHandle(InputThread);
	CloseHandle(ServerStatsThread);

	return 0;
}
