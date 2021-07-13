// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include <stdio.h>
#include "hook.h"

int StartD3DHooks()
{
    DWORD vftable_addr, DXBase,ES_addr = NULL;
    
    FILE* pFile = nullptr;
    DXBase = (DWORD)LoadLibraryA("d3d9.dll");
    while (!DXBase);
    {
        vftable_addr = hook::FindPattern(DXBase, 0x128000,
            (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00", (char *)"xx????xx????xx????");
    } 
    ES_addr = *((BYTE*)vftable_addr + 5)*0x1000000 + *((BYTE*)vftable_addr + 4)*0x10000 + *((BYTE*)vftable_addr + 3)*0x100 + *((BYTE*)vftable_addr + 2)*0x1;
    ES_addr = (DWORD)((DWORD*)ES_addr + 42);
    ES_addr = *(DWORD*)ES_addr + 0xc;

    if (AllocConsole()) {
        freopen_s(&pFile, "CONIN$", "rb", stdin);
        freopen_s(&pFile, "CONOUT$", "wb", stdout);
        freopen_s(&pFile, "CONOUT$", "wb", stderr);
        printf("vtable code addr : %08x \n", vftable_addr);    
        printf("function addr : %08x \n", ES_addr);
    }
    hook::detour(ES_addr,5);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        StartD3DHooks();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

