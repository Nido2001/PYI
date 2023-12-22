#include "SDK.h"

_Py_SetProgramName Py_SetProgramName;
_PyEval_InitThreads PyEval_InitThreads;
_PyGILState_Ensure PyGILState_Ensure;
_PyGILState_Release PyGILState_Release;
_PyRun_SimpleStringFlags PyRun_SimpleStringFlags;

void SDK::InitCPython()
{
	HMODULE hPython = nullptr;
	const char* pythonVersions[] = {
		"python39.dll",
		"python38.dll",
		"python37.dll",
		"python310.dll",
		"python311.dll"
	};

	while (hPython == nullptr) {
		printf("No valid python module was found\n");
		for (const char* version : pythonVersions) {
			HMODULE hModule = GetModuleHandleA(version);
			if (hModule) {
				hPython = hModule;
				break;
			}
		}
	}

	if (hPython)
	{
		Py_SetProgramName = (_Py_SetProgramName)(GetProcAddress(hPython, "Py_SetProgramName"));
		PyEval_InitThreads = (_PyEval_InitThreads)(GetProcAddress(hPython, "PyEval_InitThreads"));
		PyGILState_Ensure = (_PyGILState_Ensure)(GetProcAddress(hPython, "PyGILState_Ensure"));
		PyGILState_Release = (_PyGILState_Release)(GetProcAddress(hPython, "PyGILState_Release"));
		PyRun_SimpleStringFlags = (_PyRun_SimpleStringFlags)(GetProcAddress(hPython, "PyRun_SimpleStringFlags"));
	}
	else
	{
		printf("No valid python module was found\n");
	}
}