#include "source.h"

#define IOCTL_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN,0x888,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define SYMLINK L"\\DosDevices\\ioctltest"

MAL_GLOBAL g_MalGlobal;

//���� ���� 
#pragma warning(push)
#pragma warning(disable:4100)

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath) {

	UNICODE_STRING   deviceSymLink;
	UNICODE_STRING   deviceName;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	//�� ��� �Ķ���͸� ����ó�� ���� ������ ���� �߻� 
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	//����� �޽��� ���
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Driver Loaded!\n");

	//�������� ����ü �ʱ�ȭ 
	RtlZeroMemory(&g_MalGlobal, sizeof(MAL_GLOBAL));

	//����ü�� ����̹� ������Ʈ ���� 
	g_MalGlobal.DriverObject = DriverObject;

	//�����ڵ� Ÿ�� ���ڿ� �ʱ�ȭ  -> IoCreateSymbolicLink �Լ� ���ڷ� �����ڵ� Ÿ�� ���ڿ� �Ѿ���� 
	RtlInitUnicodeString(&deviceSymLink, SYMLINK);
	RtlInitUnicodeString(&deviceName, L"\\Device\\testName");

	//PDEVICE_OBJECT ���� 
	retStatus = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&deviceObject);

	//NTSTATUS ��� SUCCESS ���� �˻� 
	if (!NT_SUCCESS(retStatus)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "IoCreateDevice Fail!\n");
		return retStatus;
	}

	//����ü�� ����̽� ������Ʈ ���� 
	g_MalGlobal.DeviceObject = deviceObject;

	//�ɺ��� ��ũ ���� 
	retStatus = IoCreateSymbolicLink(&deviceSymLink, &deviceName);

	//NTSTATUS ��� SUCCESS ���� �˻� 
	if (!NT_SUCCESS(retStatus)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "IoCreateSymLink Fail!\n");
		return retStatus;
	}

	DriverObject->DriverUnload = UnloadMyDriver;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = myIOCTL;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = createHandler;

	return 0;
}

//����̹� ��ε� �Լ� 
void UnloadMyDriver(PDRIVER_OBJECT DriverObject) {
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Driver Unloaded!\n");
	if (g_MalGlobal.DeviceObject != nullptr)
	{
		//�ɺ��� ��ũ �� �ʱ�ȭ ���ְ� ���� �Ҵ� ����
		UNICODE_STRING Symlink = { 0, };
		RtlInitUnicodeString(&Symlink, SYMLINK);
		IoDeleteSymbolicLink(&Symlink);

		//����̽� ������Ʈ ���� 
		IoDeleteDevice(g_MalGlobal.DeviceObject);
	}
	return ;
}

//IOCTL ���� �Լ� 
NTSTATUS myIOCTL(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp) {
	PIO_STACK_LOCATION pStack;
	NTSTATUS retStatus = STATUS_SUCCESS;
	ULONG ControlCode;
	PWCHAR InBuf;
	PWCHAR OutBuf;
	PWCHAR OutMsg = L"Message from driver\n";
	ULONG InBufLength,OutBufLength,OutMsgLength;

	//IRP ���ο� �����ϴ� IO���� ��������ġ ��ȯ 
	pStack = IoGetCurrentIrpStackLocation(irp);

	InBufLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
	OutBufLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	//���� ���ø����̼ǿ��� ������ ��Ʈ�� �ڵ� ã�ƿ�
	ControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;

	//��Ʈ�� �ڵ忡 ���� ���� ����
	switch (ControlCode) {
	case IOCTL_MESSAGE:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "IOCTL EVENT!\n");

		//�������� �Ѿ�� �޽��� 
		InBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "data from user : %S\n",InBuf);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "meesage length : %d\n", InBufLength);

		OutMsgLength = (ULONG) wcslen(OutMsg)*2 +1;
		//������ �ѱ� �޽��� 
		OutBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;
		//�޽��� ���� 
		RtlCopyBytes(OutBuf, OutMsg, OutBufLength);
		//���޹��� outbufLen�� ���� ���޵� ���ڿ��� ���� ���Ͽ� ���޵� �޽��� ���� ����
		irp->IoStatus.Information = (OutBufLength < OutMsgLength? OutBufLength : OutMsgLength);

		break;
	}

	//IRP ���� �Ϸ� ó�� �Լ� 
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return retStatus;
}

//Create ���� �Լ� - �Լ����� ������ STATUS_SUCESS�� ��ȯ���־�� �Ѵ�. 
NTSTATUS createHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp) {
	//I/O ���� ���� 
	irp->IoStatus.Status = STATUS_SUCCESS;

	//IRP ���� �Ϸ� ó�� �Լ� 
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "CREATE EVENT!\n");
	return STATUS_SUCCESS;
}

//������/���� ���� ����  -> UNREFERENCED_PARAMETER(DriverObject) �Ƚᵵ��
#pragma warning(pop)