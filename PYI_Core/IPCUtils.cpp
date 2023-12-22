#include "IPCUtils.h"
#include <iostream>

bool CreateNamedPipeServer(HANDLE& hPipe) {
	hPipe = CreateNamedPipe(
		L"\\\\.\\pipe\\MyNamedPipe", // Pipe name
		PIPE_ACCESS_OUTBOUND,        // Pipe open mode
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // Pipe mode
		1,                           // Maximum instances
		0,                           // Out buffer size
		0,                           // In buffer size
		0,                           // Default timeout
		nullptr                      // Security attributes
	);

	if (hPipe == INVALID_HANDLE_VALUE) {
		std::cerr << "Error creating named pipe. Error code: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

bool ConnectToNamedPipe(HANDLE& hPipe) {
	if (!ConnectNamedPipe(hPipe, nullptr)) {
		DWORD dwError = GetLastError();
		if (dwError != ERROR_PIPE_CONNECTED && dwError != ERROR_NO_DATA) {
			std::cerr << "Error connecting to named pipe. Error code: " << dwError << std::endl;
			CloseHandle(hPipe);
			return false;
		}
	}

	return true;
}

bool SendMessageToPipe(HANDLE hPipe, const char* message) {
	DWORD dwBytesWritten;
	if (!WriteFile(hPipe, message, static_cast<DWORD>(strlen(message) + 1), &dwBytesWritten, nullptr)) {
		std::cerr << "Error writing to named pipe. Error code: " << GetLastError() << std::endl;
		return false;
	}

	return true;
}

void CloseNamedPipe(HANDLE hPipe) {
	CloseHandle(hPipe);
}