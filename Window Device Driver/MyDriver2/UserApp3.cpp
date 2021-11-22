#include <stdio.h>
#include <Windows.h>

// IOCTL 코드 정의 
#define IOCTL_PROCLIST CTL_CODE(FILE_DEVICE_UNKNOWN,0x999,METHOD_BUFFERED,FILE_ANY_ACCESS)

#pragma warning(disable: 4996)

int main() {
	HANDLE hHandle;
	BOOL DICret;
	WCHAR path[] = L"\\\\.\\ioctltest";
	WCHAR inputBuffer[100];
	WCHAR outputBuffer[100];
	ULONG bytesret;
	int mLen;


	//CreateFile 실행하여 Device handle 반환 
	hHandle = CreateFile(path,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hHandle == INVALID_HANDLE_VALUE) {
		printf("Open Handle ERROR\n");

	}
	wcscpy(inputBuffer, L"Sending debugging message");
	mLen = (DWORD)wcslen(inputBuffer) * 2 + 1;
	DICret = DeviceIoControl(hHandle,
		IOCTL_PROCLIST,
		&inputBuffer,
		mLen,
		&outputBuffer,
		sizeof(outputBuffer),
		&bytesret,
		NULL);

	if (!DICret) {
		printf("Device Io Control Error \n");
	}
	
	return 0;
}