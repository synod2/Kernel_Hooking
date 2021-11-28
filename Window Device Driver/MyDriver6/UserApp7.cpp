#include <stdio.h>
#include <Windows.h>

// IOCTL �ڵ� ���� 
#define IOCTL_CIDTABLE CTL_CODE(FILE_DEVICE_UNKNOWN,0xDDD,METHOD_BUFFERED,FILE_ANY_ACCESS)

#pragma warning(disable: 4996)

int main() {
	HANDLE hHandle;
	BOOL DICret;
	WCHAR path[] = L"\\\\.\\ioctltest";
	WCHAR inputBuffer[100];
	WCHAR outputBuffer[100];
	ULONG bytesret;
	int mLen;


	//CreateFile �����Ͽ� Device handle ��ȯ 
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
	wcscpy(inputBuffer, L"GET PROCESSLIST BY PSPCIDTABLE");
	mLen = (DWORD)wcslen(inputBuffer) * 2 + 1;
	DICret = DeviceIoControl(hHandle,
		IOCTL_CIDTABLE,
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