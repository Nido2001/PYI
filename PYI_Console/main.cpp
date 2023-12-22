#include "SDK.h"
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <zmq.hpp>
#include <shellapi.h>
#include <shlwapi.h>
#include <iostream>

#pragma comment(lib, "shlwapi.lib")

using namespace std;

DWORD FindChildProcess(const wchar_t* processName, DWORD currentProcessId) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (Process32First(hSnapshot, &pe32)) {
		do {
			if (wcscmp(pe32.szExeFile, processName) == 0 && pe32.th32ProcessID != currentProcessId) {
				CloseHandle(hSnapshot);
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return 0;
}

BOOL InjectDll(DWORD processId, LPCWSTR dllPath) {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (hProcess == NULL) {
		return FALSE;
	}

	size_t pathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);

	LPVOID remoteString = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (remoteString == NULL) {
		CloseHandle(hProcess);
		return FALSE;
	}

	if (!WriteProcessMemory(hProcess, remoteString, dllPath, pathSize, nullptr)) {
		VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	LPTHREAD_START_ROUTINE loadLibraryAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	if (loadLibraryAddr == nullptr) {
		VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	HANDLE remoteThread = CreateRemoteThread(hProcess, nullptr, 0, loadLibraryAddr, remoteString, 0, nullptr);
	if (remoteThread == nullptr) {
		VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return FALSE;
	}

	WaitForSingleObject(remoteThread, INFINITE);

	CloseHandle(remoteThread);
	VirtualFreeEx(hProcess, remoteString, 0, MEM_RELEASE);

	CloseHandle(hProcess);

	return TRUE;
}

bool IsProcessRunning(DWORD processId) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
	if (hProcess == nullptr) {
		return false;
	}

	DWORD exitCode;
	GetExitCodeProcess(hProcess, &exitCode);

	CloseHandle(hProcess);

	return exitCode == STILL_ACTIVE;
}

int wmain(int argc, wchar_t* argv[])
{
	LPCWSTR dllFileName = L"PYI_Core.dll";
	WCHAR dllPath[MAX_PATH];
	DWORD result = SearchPathW(nullptr, dllFileName, nullptr, MAX_PATH, dllPath, nullptr);

	if (argc < 2) {
		std::wcerr << L"No file specified." << std::endl;
		return 1;
	}

	const wchar_t* filePath = argv[1];
	const wchar_t* fileName = PathFindFileName(filePath);

	LPCWSTR commandLineArgs = nullptr;
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = SW_SHOWNORMAL;

	if (CreateProcess(
		filePath,
		const_cast<LPTSTR>(commandLineArgs),
		nullptr,
		nullptr,
		FALSE,
		CREATE_NEW_CONSOLE,
		nullptr,
		nullptr,
		&startupInfo,
		&processInfo
	)) {
		printf("Target process created successfully. Process ID: %d\n", processInfo.dwProcessId);

		DWORD childProcessId = 0;

		while (childProcessId == 0) {
			childProcessId = FindChildProcess(fileName, processInfo.dwProcessId);
			if (childProcessId == 0) {
				printf("Child not found. Retrying...\n");
				Sleep(2000);
			}
		}

		printf("Child process found. Process ID: %d\n", childProcessId);
		WaitForSingleObject(processInfo.hProcess, 2500);

		if (InjectDll(childProcessId, dllPath)) {
			printf("DLL injected successfully into the child process.\n");
		}
		else {
			printf("Failed to inject DLL into the child process.\n");
		}

		Sleep(5000);

		zmq::context_t context(1);
		zmq::socket_t socket(context, ZMQ_REQ);
		socket.connect("tcp://127.0.0.1:5555");

		zmq::pollitem_t items[] = {
			{ socket, 0, ZMQ_POLLIN, 0 }
		};

		while (true) {
			try {
				std::string message;
				std::cout << ">>";
				std::getline(std::cin, message);

				if (message == "exit") {
					break;
				}

				zmq::message_t zmqMessage(message.size());
				memcpy(zmqMessage.data(), message.c_str(), message.size());
				socket.send(zmqMessage);

				zmq::poll(items, 1, 1000);

				if (items[0].revents & ZMQ_POLLIN) {
					zmq::message_t response;
					socket.recv(&response);
				}
				else {
				}
			}
			catch (...) {
			}
		}

		Sleep(10000000);

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else {
		printf("Failed to create process. Error code: %d\n", GetLastError());
	}

	return 0;
}

