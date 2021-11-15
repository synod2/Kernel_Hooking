#include <ntddk.h>

DRIVER_UNLOAD UnloadMyDriver;

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath) {
	//�� ��� �Ķ���͸� ����ó�� ���� ������ ���� �߻� 
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	//����� �޽��� ���
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Hello World!\n");
	DriverObject->DriverUnload = UnloadMyDriver;
	return 0;
}

void UnloadMyDriver(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Driver Unloaded!\n");
	return ;
}
