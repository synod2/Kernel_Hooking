#ifndef _SOURCE_H_
#define _SOURCE_H_

#include <ntddk.h>

DRIVER_UNLOAD UnloadMyDriver;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH myIOCTL;

_Dispatch_type_(IRP_MJ_CREATE)
DRIVER_DISPATCH createHandler;

//전역적으로 활용할 오브젝트나 변수들을 위해 구조체 선언하여 사용. 
typedef struct _MAL_GLBOAL {
	PDRIVER_OBJECT DriverObject;
	PDEVICE_OBJECT DeviceObject;
}MAL_GLOBAL,*PMAL_GLOBAL;

extern MAL_GLOBAL g_MalGlobal;

#endif