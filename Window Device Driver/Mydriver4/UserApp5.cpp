#include <stdio.h>
#include <Windows.h>

// IOCTL 코드 정의 
#define IOCTL_PROCINFO CTL_CODE(FILE_DEVICE_UNKNOWN,0xAAA,METHOD_BUFFERED,FILE_ANY_ACCESS)

#pragma warning(disable: 4996)

int main(int argc,char * argv[]) {
	HANDLE hHandle;
	BOOL DICret;
	WCHAR path[] = L"\\\\.\\ioctltest";
	WCHAR inputBuffer[100];
	WCHAR outputBuffer[100];
	ULONG bytesret;
	int mLen;

	if (argc < 2) {
		printf("name [PID]");
		return 0;
	}


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
	MultiByteToWideChar(CP_ACP, 0, argv[1], strlen(argv[1]) + 1, inputBuffer, 100);
	mLen = (DWORD)wcslen(inputBuffer) * 2 + 1;
	DICret = DeviceIoControl(hHandle,
		IOCTL_PROCINFO,
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