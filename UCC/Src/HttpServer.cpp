#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <queue>

#include "HttpServer.h"
#include "httplib.h"

static httplib::Server svr;

RepcomStatsServer::RepcomStatsServer() {
	DataMutex = CreateMutex(NULL, FALSE, NULL);

	svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
		char buf[1024*64];
		RepcomStats rs = this->GetStats();
		sprintf(buf, "UCC TPS: %f\nGame TPS: %f\n", rs.UCCTickRate, rs.TickRate);
		res.set_content(buf, "text/plain");
	});
}

void RepcomStatsServer::Listen() {
	svr.listen("0.0.0.0", 8080);
}

void RepcomStatsServer::UpdateStats(RepcomStats NewStats) {
	DWORD dwWaitResult = WaitForSingleObject(DataMutex, INFINITE);
	switch (dwWaitResult) {
		// The thread got ownership of the mutex
		case WAIT_OBJECT_0:
			memcpy(&this->Stats, &NewStats, sizeof(struct RepcomStats));
			if (!ReleaseMutex(DataMutex))
			{
				std::printf("Cannot release mutex!!!\n");
			}
		// The thread got ownership of an abandoned mutex
		case WAIT_ABANDONED:
			return;
	}
}

RepcomStats RepcomStatsServer::GetStats() {
	DWORD dwWaitResult = WaitForSingleObject(DataMutex, INFINITE);
	RepcomStats RetStats;
	switch (dwWaitResult) {
		// The thread got ownership of the mutex
	case WAIT_OBJECT_0:
		memcpy(&RetStats, &this->Stats, sizeof(struct RepcomStats));
		if (!ReleaseMutex(DataMutex))
		{
			std::printf("Cannot release mutex!!!\n");
		}
		return RetStats;
		// The thread got ownership of an abandoned mutex
	case WAIT_ABANDONED:
		return RetStats;
	default:
		return RetStats;
	}
}