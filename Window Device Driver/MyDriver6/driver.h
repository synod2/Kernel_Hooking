#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <ntddk.h>
#include <stdlib.h>
//#include <winternl.h> - NtQuerySystemInformation, SYSTEM_PROCESS_INFORMATION, SYSTEM_INFORMATION_CLASS 

#define SystemProcessInformation_d 0x5
#define POOL_TAG '1gaT'
#define _CRT_SECURE_NO_WARNINGS

// IOCTL 코드 
#define IOCTL_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN,0x888,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_PROCLIST CTL_CODE(FILE_DEVICE_UNKNOWN,0x999,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_PROCINFO CTL_CODE(FILE_DEVICE_UNKNOWN,0xAAA,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_FINDHIDE CTL_CODE(FILE_DEVICE_UNKNOWN,0xBBB,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CIDTABLE CTL_CODE(FILE_DEVICE_UNKNOWN,0xDDD,METHOD_BUFFERED,FILE_ANY_ACCESS)

DRIVER_UNLOAD UnloadMyDriver;

//IRP Major 대응 함수 드라이버 디스패치 정의
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH myIOCTL;
_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH createHandler;

//DBGPRINT WRAPPING 함수
ULONG dmsg(PCHAR msg) {
	ULONG ret;
	ret = DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "===== DRIVER MESSAGE : %s ====== \n",msg);
	return ret;
};

//NtQuerySystemInformation 함수원형 선언
typedef NTSTATUS(*NTAPI ZwQuerySystemInformation_t)(
    ULONG SystemInforationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);
ZwQuerySystemInformation_t (ZwQuerySystemInformation) = nullptr;

//PsLookupProcessByProcessId 함수 원형 선언 
typedef NTSTATUS(*NTAPI PsLookupProcessByProcessId_t)(
    long int    ProcessId,
    PEPROCESS* Process
);
PsLookupProcessByProcessId_t (PsLookupProcessByProcessId) = nullptr;

//PsGetProcessImageFileName  함수 원형 선언
typedef UCHAR*(*NTAPI PsGetProcessImageFileName_t)(
    PEPROCESS Process
);
PsGetProcessImageFileName_t(PsGetProcessImageFileName) = nullptr;





void getProcessList();

//테이블 데이터 디코딩 함수 
PVOID DecodeTableData(PVOID* EncodeAddr);

//TableLayer1단계
void TableLayer1(PVOID* TablePointer);

//TableLayer2단계
void TableLayer2(PVOID* TablePointer);

//TableLayer3단계
void TableLayer3(PVOID* TablePointer);

//PspCidTable 주소 전달받아 테이블 레이어 데이터 가져와 순회
PVOID TraversePspCidTable(ULONGLONG* PspCidTable);

//PspCidTable 실제 주소 찾아오기
ULONGLONG GetPspCidTableAddresss();

//pid 전달받아 ActiveProcesslinks 반환
PLIST_ENTRY GetProcessLinksbyPID(ULONG TargetPID);

//pid 전달받아 HandleTableList 반환
PLIST_ENTRY GetHandleTableListbyPID(ULONG TargetPID);

//pid 전달받아 해당 프로세스 숨김 
ULONGLONG HideProcessByPID(ULONG targetPID);

//pid 전달받아 ActiveProcesslinks 와 HandleTableList의 FLINK BLINK 비교하여 검증
BOOLEAN CheckLinkValidate(ULONG TargetPID, ULONG BackwardPID);




//---------------------구조체 선언------------------------------

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45,
    SystemCodeIntegrityInformation = 103,
    SystemPolicyInformation = 134,
} SYSTEM_INFORMATION_CLASS;

typedef unsigned char BYTE;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    BYTE Reserved1[48];
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    PVOID Reserved2;
    ULONG HandleCount;
    ULONG SessionId;
    PVOID Reserved3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG Reserved4;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    PVOID Reserved5;
    SIZE_T QuotaPagedPoolUsage;
    PVOID Reserved6;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved7[6];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

//-------------------------------------------------------------

//NTSTATUS NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

//-------------------------------------------------------------


//전역적으로 활용할 오브젝트나 변수들을 위해 구조체 선언하여 사용. 
typedef struct _MAL_GLBOAL {
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;
}MAL_GLOBAL,*PMAL_GLOBAL;

typedef SYSTEM_PROCESS_INFORMATION SYSTEM_PROCINFO;

extern MAL_GLOBAL g_MalGlobal;

#endif