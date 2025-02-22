/*=============================================================================
	UnFile.h: General-purpose file utilities.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// Global variables.
CORE_API extern DWORD GCRCTable[];

/*----------------------------------------------------------------------------
	Byte order conversion.
----------------------------------------------------------------------------*/

// Bitfields.
#ifndef NEXT_BITFIELD
	#define NEXT_BITFIELD(b) ((b)<<1)
	#define FIRST_BITFIELD   (1)
	#define INTEL_ORDER(x)   (x)
#endif

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

#if DO_STAT
	#define STAT(x) x
#else
	#define STAT(x) {}
#endif

/*-----------------------------------------------------------------------------
	Global init and exit.
-----------------------------------------------------------------------------*/

/*
 * Global initialization.
 * appInit initializes the engine's subsystems and thus must always be the first thing that is called in an application.
 *
 * Parameters:
 * InPackage     Name of the .int file (without extension) for the application (usually just "SWRepublicCommando")
 * InCmdLine     Command line
 * InLog         Global logger
 * InError       OutputDevice for error handling
 * InWarn        FeedbackContext used for warnings and user interaction
 * ConfigFactory Function that returns an instance of FConfigCache
 * RequireConfig Passed to FConfigCache::Init but otherwise unused
 */
CORE_API void appInit(const TCHAR* InPackage, const TCHAR* InCmdLine, FOutputDevice* InLog, FOutputDeviceError* InError, FFeedbackContext* InWarn, FConfigCache*(*ConfigFactory)(), UBOOL RequireConfig);
/*
 * Pre-shutdown.
 * Called from within guarded exit code, only during non-error exits.
 */
CORE_API void appPreExit();
/*
 * Shutdown.
 * Called outside guarded exit code, during all exits (including error exits).
 */
CORE_API void appExit();

/*-----------------------------------------------------------------------------
	Logging and critical errors.
-----------------------------------------------------------------------------*/

CORE_API void appRequestExit(UBOOL Force);

CORE_API void __cdecl appFailAssert(const ANSICHAR* Expr, const ANSICHAR* File, INT Line);
/*
 * Unwind the stack.
 * Called from the unguard macro when an exception is thrown in order to create a stacktrace.
 */
CORE_API void VARARGS appUnwindf(const TCHAR* Fmt, ...);
CORE_API const TCHAR* appGetSystemErrorMessage(INT Error = 0);
CORE_API void VARARGS appDebugMessagef(const TCHAR* Fmt, ...);
/*
 * Shows a message box.
 * Type: 1 = MB_YESNO, 2 = MB_OKCANCEL, 3 = MB_OK.
 * Returns true for yes/ok, false for no/cancel.
 */
CORE_API const UBOOL VARARGS appMsgf(INT Type, const TCHAR* Fmt, ...);
CORE_API void appGetLastError();
CORE_API void EdClearLoadErrors();
CORE_API void VARARGS EdLoadErrorf(INT Type, const TCHAR* Fmt, ...);
CORE_API UBOOL appIsDebuggerPresent();

#define debugf              GLog->Logf
#define appErrorf           GError->Logf

#if DO_GUARD_SLOW
	#define debugfSlow      GLog->Logf
	#define appErrorfSlow   GError->Logf
#else
	#define debugfSlow      GNull->Logf
	#define appErrorfSlow   GNull->Logf
#endif

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

CORE_API void* appGetDllHandle(const TCHAR* DllName);
CORE_API void appFreeDllHandle(void* DllHandle);
CORE_API void* appGetDllExport(void* DllHandle, const TCHAR* ExportName);
CORE_API void appLaunchURL(const TCHAR* URL, const TCHAR* Parms = NULL, FString* Error = NULL);
CORE_API void* appCreateProc(const TCHAR* URL, const TCHAR* Parms , UBOOL bRealTime);
CORE_API UBOOL appGetProcReturnCode(void* ProcHandle, INT* ReturnCode);
CORE_API class FGuid appCreateGuid();
CORE_API void appCreateTempFilename(const TCHAR* Path, TCHAR* Result256);
CORE_API void appCleanFileCache();
CORE_API UBOOL appFindPackageFile(const TCHAR* In, const FGuid* Guid, TCHAR* Out);
CORE_API INT appCreateBitmap(const TCHAR* Pattern, INT Width, INT Height, DWORD* Data, FFileManager* FileManager = GFileManager);

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

CORE_API void appClipboardCopy(const TCHAR* Str);
CORE_API FString appClipboardPaste();

/*-----------------------------------------------------------------------------
	Guard macros for call stack display.
-----------------------------------------------------------------------------*/

//
// guard/unguardf/unguard macros.
// For showing calling stack when errors occur in major functions.
// Meant to be enabled in release builds.
//
#if defined(_DEBUG) || !DO_GUARD
	#define guard(func)   {
	#define guardFunc     {
	#define unguard       }
	#define unguardf(msg) }
#else
	#define guard(func)   {static const TCHAR __FUNC_NAME__[]=#func; try{
	#define guardFunc     {static const TCHAR __FUNC_NAME__[]=__FUNCTION__; try{
	#define unguard       }catch(TCHAR*Err){throw Err;}catch(...){appUnwindf("%s",__FUNC_NAME__); throw;}}
	#define unguardf(msg) }catch(TCHAR*Err){throw Err;}catch(...){appUnwindf("%s",__FUNC_NAME__); appUnwindf msg; throw;}}
#endif

//
// guardSlow/unguardfSlow/unguardSlow macros.
// For showing calling stack when errors occur in performance-critical functions.
// Meant to be disabled in release builds.
//
#if defined(_DEBUG) || !DO_GUARD || !DO_GUARD_SLOW
	#define guardSlow(func)     {
	#define guardFuncSlow       {
	#define unguardfSlow(msg)   }
	#define unguardSlow         }
	#define unguardfSlow(msg)   }
#else
	#define guardSlow(func)     guard(func)
	#define guardFuncSlow       guardFunc
	#define unguardSlow         unguard
	#define unguardfSlow(msg)   unguardf(msg)
#endif

//
// For throwing string-exceptions which safely propagate through guard/unguard.
//
CORE_API void VARARGS appThrowf(const TCHAR* Fmt, ...);

/*-----------------------------------------------------------------------------
	Check macros for assertions.
-----------------------------------------------------------------------------*/

//
// "check" expressions are only evaluated if enabled.
// "verify" expressions are always evaluated, but only cause an error if enabled.
//
#if DO_CHECK
	#define check(expr)  {if(!(expr)) appFailAssert(#expr, __FILE__, __LINE__);}
	#define verify(expr) {if(!(expr)) appFailAssert(#expr, __FILE__, __LINE__);}
#else
	#define check(expr) {}
	#define verify(expr) if(expr){}
#endif

//
// Check for development only.
//
#if DO_CHECK_SLOW
	#define checkSlow(expr)  {if(!(expr)) appFailAssert(#expr, __FILE__, __LINE__);}
	#define verifySlow(expr) {if(!(expr)) appFailAssert(#expr, __FILE__, __LINE__);}
#else
	#define checkSlow(expr) {}
	#define verifySlow(expr) if(expr){}
#endif

/*-----------------------------------------------------------------------------
	Timing macros.
-----------------------------------------------------------------------------*/

//
// Normal timing.
//
#if DO_CLOCK
    #define clock(Timer)   { Timer -= appCycles();}
    #define unclock(Timer) { Timer += appCycles()-12;}
#else
    #define clock(Timer) {}
    #define unclock(Timer) {}
#endif

//
// Performance critical timing.
//
#if DO_CLOCK_SLOW
	#define clockSlow(Timer) { Timer-=appCycles();}
	#define unclockSlow(Timer) { Timer+=appCycles() - 12;}
#else
	#define clockSlow(Timer) {}
	#define unclockSlow(Timer) {}
#endif

/*-----------------------------------------------------------------------------
	Text format.
-----------------------------------------------------------------------------*/

CORE_API FString appFormat(FString Src, const FConfigSection& Map);

/*-----------------------------------------------------------------------------
	Localization.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* Localize(const TCHAR* Section, const TCHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL, UBOOL Optional = 0);
CORE_API const TCHAR* LocalizeError(const TCHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL);
CORE_API const TCHAR* LocalizeProgress(const TCHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL);
CORE_API const TCHAR* LocalizeQuery(const TCHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL);
CORE_API const TCHAR* LocalizeGeneral(const TCHAR* Key, const TCHAR* Package = GPackage, const TCHAR* LangExt = NULL);

/*-----------------------------------------------------------------------------
	File functions.
-----------------------------------------------------------------------------*/

// File utilities.
CORE_API const TCHAR* appFExt(const TCHAR* Filename);
CORE_API UBOOL appUpdateFileModTime(TCHAR* Filename);
CORE_API FString appGetGMTRef();

/*-----------------------------------------------------------------------------
	OS functions.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* appCmdLine();
CORE_API const TCHAR* appBaseDir();
CORE_API const TCHAR* appPackage();
CORE_API const TCHAR* appComputerName();
CORE_API const TCHAR* appUserName();

/*-----------------------------------------------------------------------------
	Timing functions.
-----------------------------------------------------------------------------*/

CORE_API DOUBLE appSeconds();
CORE_API DOUBLE appSecondsSlow();
CORE_API void appSleep(FLOAT Seconds);
CORE_API void appSystemTime(INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec);
CORE_API QWORD appSystemTime64();
CORE_API QWORD appSystemTime64ToLocaltime(QWORD Time64, INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec);
CORE_API const TCHAR* appTimestamp(bool IncludeDate = true, bool IncludeTime = true);

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/

inline TCHAR appToUpper(TCHAR c){
	return (c < 'a' || c > 'z') ? (c) : (c + 'A' - 'a');
}

inline TCHAR appToLower(TCHAR c){
	return (c < 'A' || c > 'Z') ? (c) : (c + 'a' - 'A');
}

inline bool appIsUpper(TCHAR c){
	return c >= 'A' && c <= 'Z';
}

inline bool appIsLower(TCHAR c){
	return c >= 'a' && c <= 'z';
}

inline bool appIsAlpha(TCHAR c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool appIsDigit(TCHAR c){
	return c >= '0' && c <= '9';
}

inline bool appIsAlnum(TCHAR c){
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

inline UBOOL appIsSpace(TCHAR c){
	return c == ' ' || (c >= '\t' && c <= '\r');
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

CORE_API const ANSICHAR* appToAnsi(const TCHAR* Str, ANSICHAR* ACh = NULL);
CORE_API const UNICHAR* appToUnicode(const TCHAR* Str, UNICHAR* UCh = NULL);
CORE_API const TCHAR* appFromAnsi(const ANSICHAR* Str, TCHAR* TCh = NULL);
CORE_API const TCHAR* appFromUnicode(const UNICHAR* Str, TCHAR* TCh = NULL);
CORE_API UBOOL appIsPureAnsi(const TCHAR* Str);

CORE_API TCHAR* appStrcpy(TCHAR* Dest, const TCHAR* Src);
CORE_API INT appStrlen(const TCHAR* String);
CORE_API TCHAR* appStrstr(const TCHAR* String, const TCHAR* Find);
CORE_API TCHAR* appStrchr(const TCHAR* String, INT c);
CORE_API TCHAR* appStrcat(TCHAR* Dest, const TCHAR* Src);
CORE_API INT appStrcmp(const TCHAR* String1, const TCHAR* String2);
CORE_API INT appStricmp(const TCHAR* String1, const TCHAR* String2);
CORE_API INT appStrncmp(const TCHAR* String1, const TCHAR* String2, INT Count);
CORE_API TCHAR* appStaticString1024();

CORE_API const TCHAR* appSpc(int Num);
CORE_API TCHAR* appStrncpy(TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrncat(TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrupr(TCHAR* String);
CORE_API const TCHAR* appStrfind(const TCHAR* Str, const TCHAR* Find);
CORE_API DWORD appStrCrc(const TCHAR* Data);
CORE_API DWORD appStrCrcCaps(const TCHAR* Data);
CORE_API INT __cdecl appAtoi(const TCHAR* Str);
CORE_API TCHAR* appItoa(const INT Num);
CORE_API FLOAT appAtof(const TCHAR* Str);
CORE_API INT appStrtoi(const TCHAR* Start, TCHAR** End, INT Base);
CORE_API INT appStrnicmp(const TCHAR* A, const TCHAR* B, INT Count);
CORE_API INT appSprintf(TCHAR* Dest, const TCHAR* Fmt, ...);
CORE_API void appTrimSpaces(ANSICHAR* String);
CORE_API INT appStrPrefix(const TCHAR* Str, const TCHAR* Prefix);

CORE_API INT appGetVarArgs(TCHAR* Dest, INT Count, const TCHAR*& Fmt);

typedef int QSORT_RETURN;
typedef QSORT_RETURN(__cdecl*QSORT_COMPARE)(const void* A, const void* B);
CORE_API void __cdecl appQsort(void* Base, INT Num, INT Width, QSORT_COMPARE Compare);

//
// Case insensitive string hash function.
//
inline DWORD appStrihash(const TCHAR* Data){
	DWORD Hash = 0;

	while(*Data){
		TCHAR Ch = appToUpper(*Data++);
		BYTE  B  = Ch;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
	}

	return Hash;
}

//
// Memory hash function
//
inline DWORD appMemhash(const void* Data, DWORD Size, DWORD HashBase = 0){
	DWORD Hash = HashBase;

	while(Size){
		BYTE B = ((BYTE*)Data)[--Size];
		Hash   = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
	}

	return Hash;
}

/*-----------------------------------------------------------------------------
	Parsing functions.
-----------------------------------------------------------------------------*/

CORE_API UBOOL __cdecl ParseCommand(const TCHAR** Stream, const TCHAR* Match);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, class FName& Name);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, DWORD& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, TCHAR* Value, INT MaxLen);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, BYTE& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, SBYTE& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, _WORD& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, SWORD& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, FLOAT& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, INT& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, FString& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, QWORD& Value);
CORE_API UBOOL Parse(const TCHAR* Stream, const TCHAR* Match, SQWORD& Value);
CORE_API UBOOL ParseUBOOL(const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff);
CORE_API UBOOL ParseObject(const TCHAR* Stream, const TCHAR* Match, class UClass* Type, class UObject*& DestRes, class UObject* InParent);
CORE_API UBOOL ParseLine(const TCHAR** Stream, TCHAR* Result, INT MaxLen, UBOOL Exact = 0);
CORE_API UBOOL ParseLine(const TCHAR** Stream, FString& Resultd, UBOOL Exact = 0);
CORE_API UBOOL __cdecl ParseToken(const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape);
CORE_API UBOOL __cdecl ParseToken(const TCHAR*& Str, FString& Arg, UBOOL UseEscape);
CORE_API FString __cdecl ParseToken(const TCHAR*& Str, UBOOL UseEscape);
CORE_API void ParseNext(const TCHAR** Stream);
CORE_API UBOOL ParseParam(const TCHAR* Stream, const TCHAR* Param);

/*-----------------------------------------------------------------------------
	Array functions.
-----------------------------------------------------------------------------*/

// Core functions depending on TArray and FString.
CORE_API UBOOL appLoadFileToArray(TArray<BYTE>& Result, const TCHAR* Filename, FFileManager* FileManager = GFileManager);
CORE_API UBOOL appLoadFileToString(FString& Result, const TCHAR* Filename, FFileManager* FileManager = GFileManager);
CORE_API UBOOL appSaveArrayToFile(const TArray<BYTE>& Array, const TCHAR* Filename, FFileManager* FileManager = GFileManager);
CORE_API UBOOL appSaveStringToFile(const FString& String, const TCHAR* Filename, FFileManager* FileManager = GFileManager);

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

FORCEINLINE void* appMemmove(void* Dest, const void* Src, INT Count){
	return memmove(Dest, Src, Count);
}

FORCEINLINE INT appMemcmp(const void* Buf1, const void* Buf2, INT Count){
	return memcmp(Buf1, Buf2, Count);
}

CORE_API UBOOL appMemIsZero(const void* V, int Count);
CORE_API DWORD appMemCrc(const void* Data, INT Length, DWORD CRC = 0);
CORE_API void appMemswap(void* Ptr1, void* Ptr2, DWORD Size);

//
// C style memory allocation stubs.
//
#define appMalloc  GMalloc->Malloc
#define appFree    GMalloc->Free
#define appRealloc GMalloc->Realloc

#pragma warning(push, 0)

//
// C++ style memory allocation.
//
inline void* __cdecl operator new(size_t Size, const TCHAR* Tag){
	guardSlow(new);
	return appMalloc(Size);
	unguardSlow;
}

inline void* __cdecl operator new(size_t Size){
	guardSlow(new);
	return appMalloc(Size);
	unguardSlow;
}

inline void __cdecl operator delete(void* Ptr){
	guardSlow(delete);
	appFree(Ptr);
	unguardSlow;
}

inline void* __cdecl operator new[](size_t Size, const TCHAR* Tag){
	guardSlow(new);
	return appMalloc(Size);
	unguardSlow;
}

inline void* __cdecl operator new[](size_t Size){
	guardSlow(new);
	return appMalloc(Size);
	unguardSlow;
}

inline void __cdecl operator delete[](void* Ptr){
	guardSlow(delete);
	appFree(Ptr);
	unguardSlow;
}

// Placement-new

inline void* __cdecl operator new(size_t, void* Ptr){
	return Ptr;
}

inline void* __cdecl operator new[](size_t, void* Ptr){
	return Ptr;
}

#pragma warning(pop)

/*-----------------------------------------------------------------------------
	Math functions.
-----------------------------------------------------------------------------*/

inline FLOAT appExp(FLOAT Value){ return expf(Value); }
inline FLOAT appLoge(FLOAT Value){ return logf(Value); }
inline FLOAT appFmod(FLOAT Y, FLOAT X){ return fmodf(Y,X); }
inline FLOAT appSin(FLOAT Value){ return sinf(Value); }
inline FLOAT appAsin(FLOAT Value){ return asinf(Value); }
inline FLOAT appCos(FLOAT Value){ return cosf(Value); }
inline FLOAT appAcos(FLOAT Value){ return acosf(Value); }
inline FLOAT appTan(FLOAT Value){ return tanf(Value); }
inline FLOAT appAtan(FLOAT Value){ return atanf(Value); }
inline FLOAT appAtan2(FLOAT Y, FLOAT X) { return atan2f(Y,X); }
inline FLOAT appPow(FLOAT A, FLOAT B) {return powf(A,B); }
inline UBOOL appIsNan(FLOAT A){ return _isnan(A); }
inline INT appCeil(FLOAT Value){ return (INT)ceilf(Value); }
inline INT appRand(){ return rand(); }
inline void appRandInit(INT Seed){ srand(Seed); }
inline FLOAT appFrand(){ return rand() / (FLOAT)RAND_MAX; }

CORE_API void appSRandInit(INT Seed);
CORE_API FLOAT appSRand();
CORE_API INT appCeilFast16(FLOAT Value);
CORE_API BYTE appCeilLogTwo(DWORD Arg);

/*-----------------------------------------------------------------------------
	MD5 functions.
-----------------------------------------------------------------------------*/

//
// MD5 Context.
//
struct FMD5Context{
	DWORD state[4];
	DWORD count[2];
	BYTE buffer[64];
};

//
// MD5 functions.
// it would be cool if these were implemented as subclasses of
// FArchive.
//
CORE_API void appMD5Init(FMD5Context* context);
CORE_API void appMD5Update(FMD5Context* context, BYTE* input, INT inputLen);
CORE_API void appMD5Final(BYTE* digest, FMD5Context* context);
CORE_API void appMD5Transform(DWORD* state, BYTE* block);
CORE_API void appMD5Encode(BYTE* output, DWORD* input, INT len);
CORE_API void appMD5Decode(DWORD* output, BYTE* input, INT len);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
