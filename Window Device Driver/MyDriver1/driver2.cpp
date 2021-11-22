#include <ntddk.h>

DRIVER_UNLOAD UnloadMyDriver;

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath) {
	//미 사용 파라미터를 에러처리 하지 않으면 오류 발생 
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	//디버그 메시지 출력
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Hello World!\n");
	DriverObject->DriverUnload = UnloadMyDriver;
	return 0;
}

void UnloadMyDriver(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Driver Unloaded!\n");
	return ;
}