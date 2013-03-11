// Hook.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <WindowsX.h>



#include "../HookDll/HookDll.h"

// static HINSTANCE sHinstDll;
// typedef BOOL (*SETHOOK)(DWORD dwThread);
// SETHOOK mySetHook;

int _tmain(int argc, _TCHAR* argv[])
{

	//inject my dll
	setHook();
// 	MSG msg;
// 	GetMessage(&msg, NULL, 0, 0);
// 	MSG msg;
// 	GetMessage(&msg,(HWND)-1,0,0);
// 	printf("%x",(UINT)(msg.message));
	system("pause");
	unsetHook();
	return 0;
}

