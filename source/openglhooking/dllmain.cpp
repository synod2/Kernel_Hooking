// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#pragma once
#include <windows.h>
#include <stdio.h>
#include <GL/glut.h>
#include "draw.h"


namespace hooker {
    DWORD origin_addr = 0;
    int a = 10;
    int b = 10;
    int c = 1;
    void hookf() {
        Draw* dtpr = new Draw;
        if (a == 100)
            c *= -1;
        else if (a == 9)
            c *= -1;
        dtpr->box(a, b, 200, 200, 255, 000, 255);    
        a += c;
        b += c;
        glFlush();
    }

    void modifier(DWORD origin,DWORD newfunc,int size) {
        DWORD Protect = 0;
        DWORD newOffset = newfunc - origin +1 ; //make offset 
        VirtualProtect((LPVOID)origin, size, PAGE_EXECUTE_READWRITE, &Protect); //change page permission

        printf("origin addr : %08x \n", origin);
        printf("tramp addr : %08x \n", newfunc);
        origin_addr = origin + size;
        *((LPBYTE)origin + 0) = 0xE9;       //overwrite - JMP
        origin += 1;
        *((LPDWORD)origin + 0) = newOffset; //ADDR 4byte
        origin += 4;
        *((LPBYTE)origin) = 0x90;       //NOP

        
    }


    void Tramp() {
        __asm {
            PUSHFD //save all register and flags
            PUSHAD
            CALL hookf //call custom function 
            POPAD
            POPFD
            PUSH EBP    //origin function's asm
            MOV EBP,ESP
            SUB ESP,8
            JMP [origin_addr]     //back to origin 
        }
    }

    void hook() {
        FILE* pFile = nullptr;
        int* taddr = (int*)0x6814f4e0;
        int** faddr =(int **) *taddr ;
        int offset = 0x88 / sizeof(int);
        if (AllocConsole()) {

            freopen_s(&pFile, "CONIN$", "rb", stdin);
            freopen_s(&pFile, "CONOUT$", "wb", stdout);
            freopen_s(&pFile, "CONOUT$", "wb", stderr);

            printf("base : %08x \n", taddr);
            printf("follow address : %08x \n", *taddr);
            printf("follow pointer: %08x \n", faddr+offset);
            printf("function pointer : %08x \n", *(faddr + offset));

            modifier((DWORD)*(faddr + offset),(DWORD)Tramp, 6);
        }
    }

};

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        MessageBox(nullptr, L"injection success", L"dll injection", MB_OK);
        hooker::hook();
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

