//#pragma once
//
//#include <stdio.h>
//#include <windows.h>
//namespace hooker {
//	void hook() {
//		FILE* pFile = nullptr;
//		HMODULE dhandle = GetModuleHandle(L"opengl32.dll");
//		int* taddr = (int*) 0x6814f4e0;
//
//		if (AllocConsole()) {
//
//			freopen_s(&pFile, "CONIN$", "rb", stdin);
//			freopen_s(&pFile, "CONOUT$", "wb", stdout);
//			freopen_s(&pFile, "CONOUT$", "wb", stderr);
//
//			printf("base : %08x \n", taddr);
//			printf("follow address : %08x \n", *taddr);
//		}
//	}
//
//};