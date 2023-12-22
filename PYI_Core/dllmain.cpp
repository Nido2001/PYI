#define _CRT_SECURE_NO_WARNINGS

#include "SDK.h"
#include <iostream>
#include <string>
#include <thread>
#include <Windows.h>

SDK sdk;

DWORD WINAPI MainThread(HMODULE hModule)
{
	sdk.InitCPython();
	Py_SetProgramName(L"PyInjector");
	PyEval_InitThreads();

	PyGILState_STATE s = PyGILState_Ensure();
	PyRun_SimpleString("exec(open(r\"C:\\Users\\jenil\\source\\repos\\PYI_Console\\x64\\Release\\code.py\").read())");
	PyGILState_Release(s);

	FreeLibraryAndExitThread(hModule, 0);
	CloseHandle(hModule);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

