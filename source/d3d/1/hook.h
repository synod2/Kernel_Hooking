#pragma once
#include <windows.h>

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


    DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE* bMask, char* szMask)
    {
        for (DWORD i = 0; i < dwLen; i++) {
            if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
                return (DWORD)(dwAddress + i);
        }
        return 0;
    }

    void hookf() {
        printf(" execute hook function ");
        return;
    }

    void detour(DWORD origin, int size) {
        DWORD protect = 0;
        DWORD tmp_addr = 0;

        VirtualProtect((LPVOID)origin, size, PAGE_EXECUTE_READWRITE, &protect);

        return_addr = origin + size;
        //make trampoline
        //1. PUSHFD, PUSHAD, CALL hooker - stub1
        //2. POPAD, POPDF,  - stub1
        //3. original byte codes - memcpy from origin code
        //4. JMP original code - stub2
        BYTE stub1[] = {
            0x9C, 0x60, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x61, 0x9d
        };
        
        //set byte code
        BYTE stub2[] = {
            0xE9, 0x00, 0x00, 0x00, 0x00
        };
        LPDWORD pTramp = (LPDWORD)VirtualAlloc(0, size + sizeof(stub1) + sizeof(stub2), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        tmp_addr = (DWORD)hook::hookf - (DWORD)pTramp - 7; //relative to tramp code
        for (int i = 0; i != 4; i++) {
            stub1[3+i] = (BYTE)tmp_addr;
            tmp_addr = tmp_addr >> 8;
        }        
        
        tmp_addr = return_addr - (DWORD)pTramp - 19; //relative to tramp code
        for (int i = 0; i != 4; i++) {
            stub2[1 + i] = (BYTE)tmp_addr;
            tmp_addr = tmp_addr >> 8;
        }   

        //set trampoline
        
        if (pTramp) {
            memcpy(pTramp, stub1, sizeof(stub1)); //copy ~ stub1
            memcpy( (LPBYTE)pTramp + sizeof(stub1) , (LPVOID)origin, size); //copy origin code
            memcpy( (LPBYTE)pTramp + sizeof(stub1) + size, stub2, sizeof(stub2)); //copy origin code
        }
        else
            return;
        //overwrite 
        
        DWORD newOffset = (DWORD)pTramp - origin - 5; //relative to origin code
        printf(" tramp addr %08x \n", newOffset);
        printf(" tramp addr %08x \n", pTramp);

        *((LPBYTE)origin) = 0xE9;
        origin += 1;
        *((LPDWORD)origin) = newOffset;
        origin += 4;
        //*((LPBYTE)origin) = 0x90;
        //*((LPBYTE)origin+1) = 0x90;

    }
}