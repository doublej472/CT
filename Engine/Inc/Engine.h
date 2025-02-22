/*=============================================================================
	Engine.h: Unreal engine public header file.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#pragma once

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#include "../../Core/Inc/Core.h"

#ifndef ENGINE_API
#define ENGINE_API DLL_IMPORT
#endif

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

LINK_LIB(Engine)

#if SUPPORTS_PRAGMA_PACK
#pragma pack(push,4)
#endif

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

ENGINE_API extern class UEngine*            GEngine;
ENGINE_API extern class FMemStack           GEngineMem;
ENGINE_API extern class FMemCache           GCache;
ENGINE_API extern class UGlobalTempObjects* GGlobalTempObjects;
ENGINE_API extern FLOAT                     GEngineDeltaTime;
ENGINE_API extern FLOAT                     GEngineTime;

#define DECLARE_STATIC_UOBJECT(ObjectClass, ObjectName, ExtraJunk) \
	static ObjectClass* ObjectName = NULL; \
	if(!ObjectName ){ \
		ObjectName = ConstructObject<ObjectClass>(ObjectClass::StaticClass()); \
		GGlobalTempObjects->AddGlobalObject((UObject**)&ObjectName); \
		ExtraJunk; \
	}

/*-----------------------------------------------------------------------------
	Size of the world.
-----------------------------------------------------------------------------*/

#define WORLD_MAX       524288.0   /* Maximum size of the world */
#define HALF_WORLD_MAX  262144.0   /* Half the maximum size of the world */
#define HALF_WORLD_MAX1 262143.0   /* Half the maximum size of the world - 1*/
#define MIN_ORTHOZOOM   250.0      /* Limit of 2D viewport zoom in */
#define MAX_ORTHOZOOM   16000000.0 /* Limit of 2D viewport zoom out */

/*-----------------------------------------------------------------------------
	Engine public includes.
-----------------------------------------------------------------------------*/

#include "UnObj.h"            // Standard object definitions.
#include "UnRenderResource.h" // Render resource objects.
#include "UnPrim.h"           // Primitive class.
#include "UnModel.h"          // Model class.
#include "UnMaterial.h"       // Materials.
#include "UnTex.h"            // Texture and palette.
#include "UnAnim.h"           // Animation.
#include "EngineClasses.h"    // All actor classes.
#include "UnURL.h"            // Uniform resource locators.
#include "UnLevel.h"          // Level object.
#include "UnIn.h"             // Input system.
#include "UnPlayer.h"         // Player class.
#include "UnEngine.h"         // Unreal engine.
#include "UnGame.h"           // Unreal game engine.
#include "UnMesh.h"           // Mesh objects.
#include "UnSkeletalMesh.h"   // Skeletal model objects.
#include "UnActor.h"          // Actor inlines.
#include "UnAudio.h"          // Audio code.
#include "UnDynBsp.h"         // Dynamic Bsp objects.
#include "UnMovie.h"
#include "UnRenDev.h"         // Rendering interface definition.
#include "UnRenderUtil.h"     // Rendering utilities.
#include "UnCamera.h"         // Viewport subsystem.
#include "UnRender.h"         // High-level rendering definitions.
#include "UnPath.h"
#include "UnCDKey.h"          // CD key validation.
#include "UnNet.h"

ENGINE_API extern class UCubemapManager*    GCubemapManager;
ENGINE_API extern class FEngineStats        GEngineStats;
ENGINE_API extern class UGlobalTempObjects* GGlobalTempObjects;
ENGINE_API extern class FMatineeTools       GMatineeTools;
ENGINE_API extern class FPathBuilder        GPathBuilder;
ENGINE_API extern class FRebuildTools       GRebuildTools;
ENGINE_API extern class FStatGraph*         GStatGraph;
ENGINE_API extern class FTempLineBatcher*   GTempLineBatcher;
ENGINE_API extern class FTerrainTools       GTerrainTools;

#if SUPPORTS_PRAGMA_PACK
#pragma pack(pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
