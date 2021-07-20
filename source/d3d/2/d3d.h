#pragma once
#include <d3d9.h>
#include <stdio.h>
#include "pch.h"

namespace d3dhelper {
	
	BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam);
	HWND GetProcessWindow();
	bool GetD3D9Device(void** pTable, size_t Size);
}