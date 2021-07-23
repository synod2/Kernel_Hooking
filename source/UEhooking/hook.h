#pragma once
#include <windows.h>
#include <TlHelp32.h>

namespace hook {
    DWORD return_addr = 0;
    bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
    {
        for (; *szMask; ++szMask, ++pData, ++bMask) {
            if (*szMask == 'x' && *pData != *bMask)
                return false;
        }
        return (*szMask) == NULL;
    }


    DWORD64 FindPattern(DWORD64 dwAddress, DWORD64 dwLen, BYTE* bMask, char* szMask)
    {
        for (DWORD i = 0; i < dwLen; i++) {
            if (bDataCompare((LPBYTE)(dwAddress + i), bMask, szMask))
                return (DWORD64)(dwAddress + i);
        }
        return 0;
    }

    void hookf() {
        //printf(" execute hook function ");
        return;
    }

    const LPVOID detour(LPBYTE origin, LPBYTE dst,int size) {
        DWORD protect = 0;
        VirtualProtect((LPVOID)origin, size, PAGE_EXECUTE_READWRITE, &protect);

        //make trampoline
        // push rax
        // movabs rax, 0xCCCCCCCCCCCCCCCC
        // xchg rax, [rsp]
        // ret
        BYTE stub[] = {
            0x50, 0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x87, 0x04, 0x24, 0xC3
        };
        LPVOID tmp;
        LPDWORD pTramp = (LPDWORD)VirtualAlloc(0, size + sizeof(stub), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        tmp = &(stub[3]);
        *( ((DWORD64 *)tmp) ) = (DWORD64) (origin + size);
        //set trampoline
        if (pTramp) {
            memcpy(pTramp, origin, size); //copy original code to trampoline
            memcpy((LPBYTE)pTramp + size, stub, sizeof(stub)); //copy stub code to trampoline
        }

        //overwrite 12bytes
        *((LPWORD)origin) = 0xB848;
        tmp = (LPBYTE)origin + 2;
        *((DWORD64 * )tmp) = (DWORD64) (dst);
        tmp = (LPBYTE)tmp + 8;
        *((LPWORD)tmp) = 0xE0FF;
        tmp = (LPBYTE)tmp + 2;
        /*for (int i = 0; i != size - 12; i++ , tmp = (LPBYTE)tmp + 1)
        {            
            *((LPBYTE)tmp) = 0x90;
        }*/

        return pTramp;
    }
}