#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <ntddk.h>

DRIVER_UNLOAD UnloadMyDriver;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH myIOCTL;

_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH createHandler;

//���������� Ȱ���� ������Ʈ�� �������� ���� ����ü �����Ͽ� ���. 
typedef struct _MAL_GLBOAL {
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;
}MAL_GLOBAL,*PMAL_GLOBAL;

extern MAL_GLOBAL g_MalGlobal;

#endif