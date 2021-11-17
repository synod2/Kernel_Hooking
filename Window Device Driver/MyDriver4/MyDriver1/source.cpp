#include "source.h"

//SYMLINK 문자열 정의 
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
	UNICODE_STRING	 NtQuerySystemInformationName;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	//미 사용 파라미터를 에러처리 하지 않으면 오류 발생 
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	//디버그 메시지 출력
	dmsg("Driver Loaded!");

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
		dmsg("IoCreateDevice Fail!");
		return retStatus;
	}

	//구조체에 디바이스 오브젝트 저장 
	g_MalGlobal.DeviceObject = deviceObject;

	//심볼릭 링크 생성 
	retStatus = IoCreateSymbolicLink(&deviceSymLink, &deviceName);

	//NTSTATUS 결과 SUCCESS 인지 검사 
	if (!NT_SUCCESS(retStatus)) {
		dmsg("IoCreateSymLink Fail!");
		return retStatus;
	}

	//NtQuerySystemInformation 함수 포인터 가져오기 
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

//프로세스 목록 가져오는 함수 
void getProcessList() {
	NTSTATUS retStatus = STATUS_SUCCESS;
	ULONG BufferSize = 0;

	//전달받을 데이터 크기 구함
	retStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, NULL, 0, &BufferSize);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Sysinfo Class : %d ====== \n", (SYSTEM_INFORMATION_CLASS)SystemProcessInformation);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== BufferSize : %ld ====== \n", BufferSize);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== NT RETURN : %x ====== \n", retStatus);
	//반환값 체크 
	if (retStatus == STATUS_INFO_LENGTH_MISMATCH) {
		dmsg("Get Buffer Size Complete!");
		if (BufferSize) {
			//커널 메모리 동적할당 
			PVOID PoolMem = ExAllocatePoolWithTag(PagedPool, BufferSize, POOL_TAG);
			dmsg("Pool Memory Complete !");
			if (PoolMem) {
				//데이터 전달받음 
				retStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, PoolMem, BufferSize, &BufferSize);
				if (NT_SUCCESS(retStatus))				{
					dmsg("Get Process Info Complete!");
					//할당한 메모리 공간 SYSTEM_PROCESS_INFORMATION 구조체에 대입 
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

void getActiveProcessLinks(long int targetPID) {
	NTSTATUS retStatus = STATUS_SUCCESS;
	ULONG BufferSize = 0;
	long long int ActiveProcessLinkAddress;

	ActiveProcessLinkAddress = getAPLAbyPID(targetPID);

	//전달받을 데이터 크기 구함
	retStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, NULL, 0, &BufferSize);
	//반환값 체크 
	if (retStatus == STATUS_INFO_LENGTH_MISMATCH) {
		if (BufferSize) {
			//커널 메모리 동적할당 
			PVOID PoolMem = ExAllocatePoolWithTag(PagedPool, BufferSize, POOL_TAG);
			if (PoolMem) {
				//데이터 전달받음 
				retStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, PoolMem, BufferSize, &BufferSize);
				if (NT_SUCCESS(retStatus)) {
					//할당한 메모리 공간 SYSTEM_PROCESS_INFORMATION 구조체에 대입 
					SYSTEM_PROCINFO* ProcessEntry = (SYSTEM_PROCINFO*)PoolMem;
					do {
						if (ProcessEntry->ImageName.Length) {
							if (targetPID == (long int)ProcessEntry->UniqueProcessId) {
								
								DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DRIVER MESSAGE : PNAME - %ws | PID - %lx ====== \n", (ProcessEntry->ImageName.Buffer), (long int)ProcessEntry->UniqueProcessId);
								DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== PID ADDRESS : %p ====== \n", &(ProcessEntry->UniqueProcessId) );
								DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== APLA ADDRESS : %p ====== \n", (PVOID)ActiveProcessLinkAddress);
							}
						}
						ProcessEntry = (SYSTEM_PROCINFO*)((BYTE*)ProcessEntry + ProcessEntry->NextEntryOffset);
					} while (ProcessEntry->NextEntryOffset);
				}
			}
		}
	}
}

long long int getAPLAbyPID(long int targetPID) {
	UNICODE_STRING targetFName;
	PEPROCESS getPEP;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PVOID PsGetProcessId = nullptr;
	PVOID* offsetPointer = nullptr;
	PLIST_ENTRY APLA,tmpNode;

	int offset;

	//PsLookupProcessByProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsLookupProcessByProcessId");
	PsLookupProcessByProcessId = (PsLookupProcessByProcessId_t)MmGetSystemRoutineAddress(&targetFName);
	if (PsLookupProcessByProcessId) {
		dmsg("Get PsLookupProcessByProcessId Pointer Success");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Function Address : %lp ====== \n", NtQuerySystemInformation);
	}
	else
		dmsg("Get PsLookupProcessByProcessId Pointer Fail");

	retStatus = PsLookupProcessByProcessId(targetPID, &getPEP);

	if(retStatus == STATUS_SUCCESS)
		dmsg("PsLookupProcessByProcessId Success");
	else
		dmsg("PsLookupProcessByProcessId fail");

	//PsGetProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsGetProcessId");
	PsGetProcessId = MmGetSystemRoutineAddress(&targetFName);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== PsGetProcssId ADDRESS : %p ====== \n", (PsGetProcessId));
	
	//PsGetProcessId 함수 시작주소부터 3바이트 위치에서 2바이트 가져옴
	//PEPROCESS 시작주소로부터 해당 2바이트만큼 오프셋 떨어진 곳에 UniqueProcessId 주소 들어있음 
	offsetPointer = (PVOID*)((long long int)PsGetProcessId + 3);
	//UniqueProcessId 로부터 8바이트 떨어진 위치가 ActiveProcessLinkAddress
	offset = (short)*offsetPointer + 8;
	
	//ActiveProcessLinkAddress 가져옴
	APLA = (PLIST_ENTRY)((long long int)getPEP + offset);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== FLINK : %p ====== \n", (APLA->Flink));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== BLINK : %p ====== \n", (APLA->Blink));

	//각 프로세스별 Flink , Blink 교체
	//Flink 따라가서 해당 프로세스의 Blink를 현재 프로세스의 Blink로 교체
	//Blink 따라가서 해당 프로세스의 Flink를 현재 프로세스의 Flink로 교체
	tmpNode = APLA->Flink;
	tmpNode->Blink = APLA->Blink;
	tmpNode = APLA->Blink;
	tmpNode->Flink = APLA->Flink;

	return (long long int)getPEP + offset;
}


//드라이버 언로드 함수 
void UnloadMyDriver(PDRIVER_OBJECT DriverObject) {
	dmsg("Driver Unloaded!");
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
		dmsg("IOCTL EVENT!");

		//유저에서 넘어온 메시지 
		InBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DATA FROM USER : %S ====== \n", InBuf);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== MESSAGE LENGTH : %d ====== \n", InBufLength);
		
		OutMsgLength = (ULONG) wcslen(OutMsg)*2 +1;
		//유저로 넘길 메시지 
		OutBuf = (PWCHAR) irp->AssociatedIrp.SystemBuffer;
		//메시지 복사 
		RtlCopyBytes(OutBuf, OutMsg, OutBufLength);
		//전달받은 outbufLen과 실제 전달될 문자열의 길이 비교하여 전달될 메시지 길이 조정
		irp->IoStatus.Information = (OutBufLength < OutMsgLength? OutBufLength : OutMsgLength);

		break;
	case IOCTL_PROCLIST : 
		dmsg("PROCLIST!");
		getProcessList();
		break;
	case IOCTL_PROCINFO : 
		dmsg("PROCINFO!");
		//유저에서 넘어온 메시지 
		InBuf = (PWCHAR)irp->AssociatedIrp.SystemBuffer;

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DATA FROM USER : %S ====== \n", InBuf);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== MESSAGE LENGTH : %d ====== \n", InBufLength);
		getActiveProcessLinks ( (long int) _wtoi(InBuf) );
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
	
	dmsg("CREATE EVENT!");
	return STATUS_SUCCESS;
}

//컴파일/빌드 오류 무시  -> UNREFERENCED_PARAMETER(DriverObject) 안써도됨
#pragma warning(pop)