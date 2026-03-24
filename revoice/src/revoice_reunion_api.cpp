#include "precompiled.h"

IReunionApi* g_ReunionApi;

bool Revoice_ReunionApi_Init()
{
	g_ReunionApi = (IReunionApi *)g_RehldsFuncs->GetPluginApi("reunion");

	if (g_ReunionApi == NULL) {
		LCPrintf(false, "Reunion not found, running without protocol detection\n");
		return true;
	}

	if (g_ReunionApi->version_major != REUNION_API_VERSION_MAJOR) {
		LCPrintf(true, "Reunion API major version mismatch; expected %d, real %d\n", REUNION_API_VERSION_MAJOR, g_ReunionApi->version_major);
		g_ReunionApi = NULL;
		return true;
	}

	if (g_ReunionApi->version_minor < REUNION_API_VERSION_MINOR) {
		LCPrintf(true, "Reunion API minor version mismatch; expected at least %d, real %d\n", REUNION_API_VERSION_MINOR, g_ReunionApi->version_minor);
		g_ReunionApi = NULL;
		return true;
	}

	return true;
}
