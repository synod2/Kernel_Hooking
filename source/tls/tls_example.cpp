#include <stdio.h>
#include "windows.h"


//linker spec
#ifdef _M_IX86
#pragma comment (linker, "/INCLUDE:__tls_used")
#else
#pragma comment (linker, "/INCLUDE:_tls_used")
#endif
EXTERN_C
#ifdef _M_X64
#pragma const_seg (".CRT$XLB")
const
#else
#pragma data_seg (".CRT$XLB")
#endif
//end linker


void NTAPI __stdcall tls_callback(PVOID, DWORD dwReason, PVOID Reserved)
{
	printf("TLS Function Called\n");
}
//tls import
PIMAGE_TLS_CALLBACK p_thread_callback = tls_callback;
#pragma data_seg ()
#pragma const_seg ()
//end 


int main() {
	char a = 0;
	while (1)
	{
		printf("main function called\n");
		scanf("%c",&a);
	}
}