#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct RepcomStats {
	float UCCTickRate = 0.0f;
	float TickRate = 0.0f;
};

class RepcomStatsServer {
private:
	RepcomStats Stats;
	HANDLE DataMutex;
public:
	RepcomStatsServer();
	void Listen();
	void UpdateStats(RepcomStats Stats);
	RepcomStats GetStats();
};