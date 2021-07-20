// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다
#include "hook.h"
#include <d3d9.h>
#include <stdio.h>

namespace d3dhelper {
	static HWND window;
	void* dtable[119];
	BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
		DWORD pid;//current process's PID
		GetWindowThreadProcessId(handle, &pid);

		if (GetCurrentProcessId() != pid)
			return TRUE;
		window = handle;
		return FALSE;
	}

	HWND GetProcessWindow() {
		window = NULL;
		EnumWindows(EnumWindowsCallback, NULL);
		return window;
	}

	bool GetD3D9Device(void** pTable, size_t Size) {
		if (!pTable)
			return false;

		IDirect3D9* pd3d = Direct3DCreate9(D3D_SDK_VERSION);

		if (!pd3d)
			return false;
		IDirect3DDevice9* pdd = NULL; //Dummy Device's pointer

		/*if (!GetProcessWindow()) {
			printf("HWND not found \n");
			return false;
		}*/
		//dummy device's options
		D3DPRESENT_PARAMETERS d3dpp = {}; //parameters
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = false;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = window; //get process handle

		//Create Dummy Device
		HRESULT ddc = pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pdd);

		if (ddc != S_OK)
		{
			// may fail in windowed fullscreen mode, trying again with windowed mode
			d3dpp.Windowed = true;

			ddc = pd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pdd);
			printf("create device success\n");
			if (ddc != S_OK)
			{
				printf("create device fail\n");
				pd3d->Release();
				return false;
			}

		}

		memcpy(pTable, *reinterpret_cast<void***>(pdd), Size);
		printf("vtable  : %08x", *pTable);
		pdd->Release();
		pd3d->Release();
		return true;
	}

}
int StartD3DHooks()
{
    DWORD vftable_addr, DXBase,ES_addr = NULL;
    void* dtable[119];
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

    //dummy device hooking
    if (d3dhelper::GetD3D9Device(dtable, sizeof(d3dhelper::dtable))) {
        //DWORD pEndScene = (DWORD)dtable[42];
        if (AllocConsole())
            printf("dummy Endscene addr : %08x", dtable[42]);
    }
    else {
        if (AllocConsole())
            printf("D3D Device getting error");
    }
    
    


    //hook::detour(ES_addr,5);
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

