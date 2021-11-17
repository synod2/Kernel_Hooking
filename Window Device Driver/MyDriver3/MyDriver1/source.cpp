#include "source.h"

// IOCTL �ڵ� ���� 
#define IOCTL_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN,0x888,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_PROCLIST CTL_CODE(FILE_DEVICE_UNKNOWN,0x999,METHOD_BUFFERED,FILE_ANY_ACCESS)

//SYMLINK ���ڿ� ���� 
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
	UNICODE_STRING	 NtQuerySystemInformationName;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	//�� ��� �Ķ���͸� ����ó�� ���� ������ ���� �߻� 
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	//����� �޽��� ���
	dmsg("Driver Loaded!");

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
		dmsg("IoCreateDevice Fail!");
		return retStatus;
	}

	//����ü�� ����̽� ������Ʈ ���� 
	g_MalGlobal.DeviceObject = deviceObject;

	//�ɺ��� ��ũ ���� 
	retStatus = IoCreateSymbolicLink(&deviceSymLink, &deviceName);

	//NTSTATUS ��� SUCCESS ���� �˻� 
	if (!NT_SUCCESS(retStatus)) {
		dmsg("IoCreateSymLink Fail!");
		return retStatus;
	}

	//NtQuerySystemInformation �Լ� ������ �������� 
	RtlInitUnicodeString(&NtQuerySystemInformationName, L"ZwQuerySystemInformation");
	NtQuerySystemInformation = (NtQuerySystemInformation_t) MmGetSystemRoutineAddress(&NtQuerySystemInformationName);
	if (NtQuerySystemInformation) {
		dmsg("Get NtQuerySystemInformation Pointer Success");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Function Address : %lp ====== \n", NtQuerySystemInformation);
	}
	else 
		dmsg("Get NtQuerySystemInformation Pointer Fail");

	DriverObject->DriverUnload = UnloadMyDriver;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = myIOCTL;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = createHandler;

	return 0;
}

//���μ��� ��� �������� �Լ� 
void getProcessList() {
	NTSTATUS retStatus = STATUS_SUCCESS;
	ULONG BufferSize = 0;

	//���޹��� ������ ũ�� ����
	retStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, NULL, 0, &BufferSize);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Sysinfo Class : %d ====== \n", (SYSTEM_INFORMATION_CLASS)SystemProcessInformation);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== BufferSize : %ld ====== \n", BufferSize);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== NT RETURN : %x ====== \n", retStatus);
	//��ȯ�� üũ 
	if (retStatus == STATUS_INFO_LENGTH_MISMATCH) {
		dmsg("Get Buffer Size Complete!");
		if (BufferSize) {
			//Ŀ�� �޸� �����Ҵ� 
			PVOID PoolMem = ExAllocatePoolWithTag(PagedPool, BufferSize, POOL_TAG);
			dmsg("Pool Memory Complete !");
			if (PoolMem) {
				//������ ���޹��� 
				retStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, PoolMem, BufferSize, &BufferSize);
				if (NT_SUCCESS(retStatus))				{
					dmsg("Get Process Info Complete!");
					//�Ҵ��� �޸� ���� SYSTEM_PROCESS_INFORMATION ����ü�� ���� 
					SYSTEM_PROCINFO *ProcessEntry = (SYSTEM_PROCINFO*)PoolMem;
					do {
						if (ProcessEntry->ImageName.Length) {
							DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DRIVER MESSAGE : PNAME - %ws | PID - %lx ====== \n",(ProcessEntry->ImageName.Buffer), (long int)ProcessEntry->UniqueProcessId);
						}
						
						ProcessEntry = (SYSTEM_PROCINFO *)((BYTE*)ProcessEntry + ProcessEntry->NextEntryOffset);
					} while (ProcessEntry->NextEntryOffset);
				}
			}
		}
	}
}


//����̹� ��ε� �Լ� 
void UnloadMyDriver(PDRIVER_OBJECT DriverObject) {
	dmsg("Driver Unloaded!");
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
		dmsg("IOCTL EVENT!");

		//�������� �Ѿ�� �޽��� 
		InBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DATA FROM USER : %S ====== \n", InBuf);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== MESSAGE LENGTH : %d ====== \n", InBufLength);
		
		OutMsgLength = (ULONG) wcslen(OutMsg)*2 +1;
		//������ �ѱ� �޽��� 
		OutBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;
		//�޽��� ���� 
		RtlCopyBytes(OutBuf, OutMsg, OutBufLength);
		//���޹��� outbufLen�� ���� ���޵� ���ڿ��� ���� ���Ͽ� ���޵� �޽��� ���� ����
		irp->IoStatus.Information = (OutBufLength < OutMsgLength? OutBufLength : OutMsgLength);

		break;
	case IOCTL_PROCLIST : 
		dmsg("PROCLIST!");
		getProcessList();
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
	
	dmsg("CREATE EVENT!");
	return STATUS_SUCCESS;
}

//������/���� ���� ����  -> UNREFERENCED_PARAMETER(DriverObject) �Ƚᵵ��
#pragma warning(pop)