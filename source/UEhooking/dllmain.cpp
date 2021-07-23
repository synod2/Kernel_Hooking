// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다
#include "hook.h"
#include <stdio.h>
#include "SDK.h"

tProcessEvent oProcessEvent = NULL;
signed int __stdcall hProcessEvent(UObject* pCallObjcet, UFunction* pUFunc, void* pParms, void* pResult)
{
	//printf("hooking successful\n");
	return oProcessEvent(pCallObjcet, pUFunc, pParms, pResult);
}

int HookMain()
{
	DWORD64 PE_addr, ES_addr = NULL;
	void* dtable[119];
	FILE* pFile = nullptr;
	LPWSTR mName = (LPWSTR)"pname";
	
	DWORD64 BaseAddress = (DWORD64)GetModuleHandle(mName)+0x1000;
	
	if (BaseAddress)
	{
		PE_addr = hook::FindPattern(BaseAddress, 0x3d10000,
			(PBYTE)"\x8b\x41\x0c\x45\x00\xf6\x3b\x05", (char*)"xxxx?xxx");
		PE_addr -= 0x54;
	}



	if (AllocConsole()) {
		freopen_s(&pFile, "CONIN$", "rb", stdin);
		freopen_s(&pFile, "CONOUT$", "wb", stdout);
		freopen_s(&pFile, "CONOUT$", "wb", stderr);
		printf(" BaseAddress : %p \n", BaseAddress);
		if (PE_addr)
			printf(" Process Event addr : %p \n", PE_addr); 
		else
			printf(" Process Event found fail\n");
	}
	oProcessEvent = (tProcessEvent) hook::detour((LPBYTE)PE_addr,(LPBYTE)hProcessEvent,12);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		HookMain();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

