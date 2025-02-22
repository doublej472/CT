#include "../../Core/Inc/Core.h"
#include "../../Core/Inc/FOutputDeviceFile.h"
#include "../../Core/Inc/FOutputDeviceWindowsError.h"
#include "../../Core/Inc/FFeedbackContextCmd.h"
#include "../../Core/Inc/FConfigCacheIni.h"

INT UServerCommandletMain(); // Defined in ServerCommandlet.cpp

static void ShowBanner(FOutputDevice& Out){
	Out.Log("=======================================");
	Out.Log("ucc.exe for Star Wars Republic Commando");
	Out.Log("made by Leon0628");
	Out.Log("=======================================");
	Out.Log("");
}

static void ShowCommandletHelp(UCommandlet* Commandlet, FOutputDevice& Out){
	Commandlet->LoadLocalized();
	Out.Log(Commandlet->HelpOneLiner);

	if(Commandlet->HelpUsage.Len() > 0){
		Out.Log("");
		Out.Log("Usage:");
		Out.Logf("    ucc %s", *Commandlet->HelpUsage);
	}

	if(Commandlet->HelpParm[0].Len() > 0){
		Out.Log("");
		Out.Log("Parameters:");

		for(INT i = 0; i < ARRAY_COUNT(Commandlet->HelpParm); ++i){
			if(Commandlet->HelpParm[i].Len() > 0 || Commandlet->HelpDesc[i].Len() > 0)
				Out.Logf("    %-20s %s", *Commandlet->HelpParm[i], *Commandlet->HelpDesc[i]);
			else
				break;
		}
	}

	if(Commandlet->HelpWebLink.Len() > 0){
		Out.Log("");
		Out.Log("For more info, see");
		Out.Logf("    %s", *Commandlet->HelpWebLink);
	}
}

static FString ResolveCommandletClassName(const FString& ClassName, const TArray<FRegistryObjectInfo>& List){
	for(int i = 0; i < List.Num(); ++i){ // Look Token up in list and autocomplete class name if found
		FString FullName = List[i].Object;
		FString ShortName = FullName;

		while(ShortName.InStr(".") >= 0) // Remove package name so that only class name remains
			ShortName = ShortName.Mid(ShortName.InStr(".") + 1);

		if(ClassName == FullName || ClassName + "Commandlet" == FullName ||  // Check against "PackageName.ClassName (+ Commandlet)"
		   ClassName == ShortName || ClassName + "Commandlet" == ShortName){ // Check against "ClassName (+ Commandlet)"
			return List[i].Object;
		}
	}

	return ClassName;
}

int __cdecl main(int argc, char** argv){
	GIsStarted = 1;
	int ExitCode = EXIT_SUCCESS;
	FOutputDeviceFile Log;
	FOutputDeviceWindowsError Error;
	FFeedbackContextCmd Warn;

	try{
		GIsGuarded = 1;

		FString CmdLine;

		for(int i = 1; i < argc; ++i)
			CmdLine += FString(argv[i]) + " ";

		appInit("SWRepublicCommando", *CmdLine, &Log, &Error, &Warn, FConfigCacheIni::Factory, 1);
		UObject::SetLanguage("int");
		// Initialize global state
		GIsUCC = GIsClient = GIsServer = GIsEditor = GIsScriptable = GLazyLoad = 1;

		if(argc > 1){
			bool ShowHelp = appStricmp(argv[1], "help") == 0;
			FString ClassName = ShowHelp ? (argc > 2 ? argv[2] : "") : argv[1];
			TArray<FRegistryObjectInfo> List;
			DWORD LoadFlags = LOAD_NoWarn | LOAD_Quiet;

			UObject::GetRegistryObjects(List, UClass::StaticClass(), UCommandlet::StaticClass(), 0); // Load list of commandlets declared in .int files

			if(ShowHelp){
				// To allow loading localizations GIsEditor must be set to false and the Core package must be loaded to register the localizable properties
				GIsEditor = 0;
				UObject::LoadPackage(NULL, "Core", LOAD_NoFail);

				bool ShowSpecializedHelp = ClassName.Len() > 0;

				if(ShowSpecializedHelp){ // Show detailed help for a specific commandlet
					bool FoundSpecializedHelp = false;

					for(TArray<FRegistryObjectInfo>::TIterator It(List); It; ++It){
						if(It->Object == ResolveCommandletClassName(ClassName, List)){
							UClass* Class = LoadClass<UCommandlet>(NULL, *It->Object, NULL, LoadFlags, NULL);

							if(Class){
								ShowCommandletHelp(static_cast<UCommandlet*>(Class->GetDefaultObject()), Warn);
								FoundSpecializedHelp = true;
							}

							break;
						}
					}

					if(!FoundSpecializedHelp){
						// Show help for a commandlet that is not in the registry objects list
						UClass* Class = LoadClass<UCommandlet>(NULL, *ClassName, NULL, LoadFlags, NULL);

						if(!Class)
							Class = LoadClass<UCommandlet>(NULL, *(ClassName + "Commandlet"), NULL, LoadFlags, NULL);

						if(Class)
							ShowCommandletHelp(static_cast<UCommandlet*>(Class->GetDefaultObject()), Warn);
						else
							Warn.Logf("    Unable to show help: Commandlet %s not found", *ClassName);
					}
				}else{ // Show general help for all commandlets
					ShowBanner(Warn);
					Warn.Log("Usage:");
					Warn.Log("    ucc <command> <parameters>");
					Warn.Log("");
					Warn.Log("Registered commands:");

					for(TArray<FRegistryObjectInfo>::TIterator It(List); It; ++It){
						UClass* Class = LoadClass<UCommandlet>(NULL, *It->Object, NULL, LoadFlags, NULL);

						if(Class){
							UCommandlet* Default = static_cast<UCommandlet*>(Class->GetDefaultObject());
							Default->LoadLocalized();
							Warn.Logf("    %-20s %s", *Default->HelpCmd, *Default->HelpOneLiner);
						}
					}

					Warn.Log("    help <command>       Get help on a command");
				}
			}else{ // Execute commandlet
				ClassName = ResolveCommandletClassName(ClassName, List);

				if(ClassName == "Editor.Make" || ClassName == "Editor.MakeCommandlet")
					LoadFlags |= LOAD_DisallowFiles;

				UClass* Class = LoadClass<UCommandlet>(NULL, *ClassName, NULL, LoadFlags, NULL);

				if(!Class) // If class failed to load append "Commandlet" and try again
					Class = LoadClass<UCommandlet>(NULL, *(ClassName + "Commandlet"), NULL, LoadFlags, NULL);

				if(Class){
					UCommandlet* Default = Cast<UCommandlet>(Class->GetDefaultObject());

					if(Default->ShowBanner)
						ShowBanner(Warn);

					Warn.Logf("Executing %s", Class->GetFullName());
					Warn.Log("");

					GIsClient = Default->IsClient;
					GIsEditor = Default->IsEditor;
					GIsServer = Default->IsServer;
					GLazyLoad = Default->LazyLoad;

					if(GIsEditor){
						List.Empty();
						UObject::GetRegistryObjects(List, UClass::StaticClass(), UExporter::StaticClass(), 0);

						for(INT i = 0; i < List.Num(); i++)
							LoadClass<UExporter>(NULL, *List[i].Object, NULL, LoadFlags, NULL);

						List.Empty();
						UObject::GetRegistryObjects(List, UClass::StaticClass(), UFactory::StaticClass(), 0);

						for(INT i = 0; i < List.Num(); i++)
							LoadClass<UFactory>(NULL, *List[i].Object, NULL, LoadFlags, NULL);
					}

					// Contains only the command-line options that are passed to the commandlet
					FString CommandletCmdLine;

					for(int i = 2; i < argc; ++i)
						CommandletCmdLine += FString(argv[i]) + " ";

					UCommandlet* Commandlet = ConstructObject<UCommandlet>(Class);
					Commandlet->InitExecution();
					Commandlet->ParseParms(*CommandletCmdLine);

					if(Default->LogToStdout){ // Redirect commandlet output to console
						Warn.AuxOut = GLog;
						GLog = &Warn;
					}

					if(ClassName == "Editor.Make" || ClassName == "Editor.MakeCommandlet"){
						// Print full path of source files in error messages
						if(ParseParam(appCmdLine(), "FullSourcePath") && (GSys->SourcePath.Len() < 2 || GSys->SourcePath[1] != ':'))
							GSys->SourcePath = FStringTemp(appBaseDir()) * GSys->SourcePath;

						if(ParseParam(appCmdLine(), "NoIncremental") || ParseParam(appCmdLine(), "All")){
							Warn.Log("Full rebuild");

							FConfigSection* ConfigSection = GConfig->GetSectionPrivate("Editor.EditorEngine", 0, 0);

							if(ConfigSection){
								TArray<FConfigString> Packages;

								ConfigSection->MultiFind("EditPackages", Packages);

								for(TArray<FConfigString>::TIterator It(Packages); It; ++It){
									FFilename Filename = *(FStringTemp(appBaseDir()) * *It + ".u");

									GLog->Logf("Deleting %s", *Filename.GetCleanFilename());
									GFileManager->Delete(*Filename, 1);
								}
							}
						}

						// Load classes that are specified in the config to avoid 'Superclass not found' errors
						FConfigSection* Section = GConfig->GetSectionPrivate("UCC.Make", 0, 1);

						if(Section){
							TArray<FConfigString> Values;
							Section->MultiFind(NAME_AutoLoad, Values);

							for(INT i = 0; i < Values.Num(); ++i){
								Warn.Logf("AutoLoad: %s", *Values[i]);

								if(!LoadObject<UObject>(NULL, *Values[i], NULL, LOAD_NoWarn | LOAD_Quiet, NULL))
									Warn.Logf(NAME_Warning, "AutoLoad: Failed to load '%s'", *Values[i]);
							}
						}
					}

					if(ClassName == "Engine.Server" || ClassName == "Engine.ServerCommandlet")
						ExitCode = UServerCommandletMain(); // The ServerCommandlet has a special Main function
					else
						ExitCode = CommandletMain(Commandlet, CommandletCmdLine);

					if(Default->ShowErrorCount){
						Warn.Log("");
						Warn.Logf("%s - %i error(s), %i warning(s)", Warn.ErrorCount == 0 ? "Success" : "Failure", Warn.ErrorCount, Warn.WarningCount);
					}

					if(Warn.ErrorCount > 0 || Warn.WarningCount > 0)
						ExitCode = EXIT_FAILURE;

					if(Default->LogToStdout){
						Warn.AuxOut = NULL;
						GLog = &Log;
					}
				}else{
					ShowBanner(Warn);
					Warn.Logf("Commandlet %s not found", argv[1]);
				}
			}
		}else{
			ShowBanner(Warn);
			Warn.Log("Use ucc help for a list of commands");
		}

		appPreExit();

		GIsGuarded = 0;
	}catch(...){
		GIsGuarded = 0;
		GLog = &Log;
		ExitCode = EXIT_FAILURE;
		Error.HandleError();
	}

	appExit();

	return ExitCode;
}
