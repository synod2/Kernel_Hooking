//IOCTL
#include "source.h"

#define IOCTL_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN,0x888,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define SYMLINK L"\\DosDevices\\ioctltest"

MAL_GLOBAL g_MalGlobal;

//오류 무시 
#pragma warning(push)
#pragma warning(disable:4100)

extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath) {

	UNICODE_STRING   deviceSymLink;
	UNICODE_STRING   deviceName;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	//미 사용 파라미터를 에러처리 하지 않으면 오류 발생 
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	//디버그 메시지 출력
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Driver Loaded!\n");

	//전역변수 구조체 초기화 
	RtlZeroMemory(&g_MalGlobal, sizeof(MAL_GLOBAL));

	//구조체에 드라이버 오브젝트 저장 
	g_MalGlobal.DriverObject = DriverObject;

	//유니코드 타입 문자열 초기화  -> IoCreateSymbolicLink 함수 인자로 유니코드 타입 문자열 넘어가야함 
	RtlInitUnicodeString(&deviceSymLink, SYMLINK);
	RtlInitUnicodeString(&deviceName, L"\\Device\\testName");

	//PDEVICE_OBJECT 생성 
	retStatus = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&deviceObject);

	//NTSTATUS 결과 SUCCESS 인지 검사 
	if (!NT_SUCCESS(retStatus)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "IoCreateDevice Fail!\n");
		return retStatus;
	}

	//구조체에 디바이스 오브젝트 저장 
	g_MalGlobal.DeviceObject = deviceObject;

	//심볼릭 링크 생성 
	retStatus = IoCreateSymbolicLink(&deviceSymLink, &deviceName);

	//NTSTATUS 결과 SUCCESS 인지 검사 
	if (!NT_SUCCESS(retStatus)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "IoCreateSymLink Fail!\n");
		return retStatus;
	}

	DriverObject->DriverUnload = UnloadMyDriver;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = myIOCTL;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = createHandler;

	return 0;
}

//드라이버 언로드 함수 
void UnloadMyDriver(PDRIVER_OBJECT DriverObject) {
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Driver Unloaded!\n");
	if (g_MalGlobal.DeviceObject != nullptr)
	{
		//심볼릭 링크 값 초기화 해주고 변수 할당 해제
		UNICODE_STRING Symlink = { 0, };
		RtlInitUnicodeString(&Symlink, SYMLINK);
		IoDeleteSymbolicLink(&Symlink);

		//디바이스 오브젝트 삭제 
		IoDeleteDevice(g_MalGlobal.DeviceObject);
	}
	return ;
}

//IOCTL 대응 함수 
NTSTATUS myIOCTL(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp) {
	PIO_STACK_LOCATION pStack;
	NTSTATUS retStatus = STATUS_SUCCESS;
	ULONG ControlCode;
	PWCHAR InBuf;
	PWCHAR OutBuf;
	PWCHAR OutMsg = L"Message from driver\n";
	ULONG InBufLength,OutBufLength,OutMsgLength;

	//IRP 내부에 존재하는 IO스택 포인터위치 반환 
	pStack = IoGetCurrentIrpStackLocation(irp);

	InBufLength = pStack->Parameters.DeviceIoControl.InputBufferLength;
	OutBufLength = pStack->Parameters.DeviceIoControl.OutputBufferLength;

	//유저 어플리케이션에서 전달한 컨트롤 코드 찾아옴
	ControlCode = pStack->Parameters.DeviceIoControl.IoControlCode;

	//컨트롤 코드에 따른 동작 대응
	switch (ControlCode) {
	case IOCTL_MESSAGE:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "IOCTL EVENT!\n");

		//유저에서 넘어온 메시지 
		InBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "data from user : %S\n",InBuf);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "meesage length : %d\n", InBufLength);

		OutMsgLength = (ULONG) wcslen(OutMsg)*2 +1;
		//유저로 넘길 메시지 
		OutBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;
		//메시지 복사 
		RtlCopyBytes(OutBuf, OutMsg, OutBufLength);
		//전달받은 outbufLen과 실제 전달될 문자열의 길이 비교하여 전달될 메시지 길이 조정
		irp->IoStatus.Information = (OutBufLength < OutMsgLength? OutBufLength : OutMsgLength);

		break;
	}

	//IRP 동작 완료 처리 함수 
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return retStatus;
}

//Create 대응 함수 - 함수동작 성공시 STATUS_SUCESS를 반환해주어야 한다. 
NTSTATUS createHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp) {
	//I/O 상태 변경 
	irp->IoStatus.Status = STATUS_SUCCESS;

	//IRP 동작 완료 처리 함수 
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "CREATE EVENT!\n");
	return STATUS_SUCCESS;
}

//컴파일/빌드 오류 무시  -> UNREFERENCED_PARAMETER(DriverObject) 안써도됨
#pragma warning(pop)