#include "../Inc/ModMPGame.h"

#include "../../Core/Inc/FOutputDeviceFile.h"

class UExportPathsCommandlet : public UCommandlet {
	DECLARE_CLASS(UExportPathsCommandlet, UCommandlet, 0, ModMPGame);

	void StaticConstructor() {
		LogToStdout = 1;
		IsServer = 1;
		IsClient = 1;
		IsEditor = 1;
		LazyLoad = 1;
		ShowErrorCount = 0;
		ShowBanner = 0;
	}

	virtual INT Main(const TCHAR* Parms) {
		FString MapName;

		if (Parse(Parms, "map=", MapName)) {
			UPackage* Package = UObject::LoadPackage(NULL, *MapName, LOAD_NoFail);
			ULevel* Level = NULL;

			foreachobj(ULevel, It) {
				if (It->IsIn(Package)) {
					Level = *It;

					break;
				}
			}

			if (Level) {
				ABotSupport* BotSupport = Cast<ABotSupport>(Level->SpawnActor(ABotSupport::StaticClass()));

				if (BotSupport)
					BotSupport->ExportPaths();
				else
					GWarn->Log(NAME_Error, "Unable to export paths");
			}
			else {
				GWarn->Logf(NAME_Error, "Package '%s' is not a map", *MapName);
			}
		}
		else {
			GWarn->Log(NAME_Error, "Map to export paths from must be specified with 'map=<MapName>'");
		}

		return 0;
	}
};

IMPLEMENT_CLASS(UExportPathsCommandlet);