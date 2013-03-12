// HookDll.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <WindowsX.h>
#include <stdio.h>
#include <iostream>

using namespace std;

#define HOOKAPI  __declspec(dllexport)
#include "HookDll.h"

LRESULT WINAPI getMsgProc(int nCode, WPARAM wParam, LPARAM lParam);

#pragma data_seg("Shared")
DWORD gDwThreadId = 0;
HHOOK gHHook = NULL;
HWND gHandle = NULL;
RECT gRect;
POINT gOffset;
const int gNumX = 9;
const int gNumY = 9;
const int gLength = 16;
#pragma data_seg()

#pragma comment(linker, "/section:Shared,rws")

HINSTANCE gHinstDll = NULL;

BYTE gBlock[gNumX*gNumY]={10};

BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		gHinstDll = hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

bool clickBlock(int x,int y)
{
//	GetWindowRect(gHandle,&gRect);
	int xpos = gOffset.x + 16*x;
	int ypos = gOffset.y + 16*y;
	LPARAM lParam = MAKELONG(xpos,ypos);
	SendMessage(gHandle,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
	if(GetLastError())
		return false;
	SendMessage(gHandle,WM_LBUTTONUP,0,lParam);
	if(GetLastError())
		return false;
	return true;
}

void updateMap(HDC hDc)
{
	COLORREF clr;
	int oriX = gOffset.x + gRect.left + 3 + 8;
	int oriY = gOffset.y + gRect.top + 48 + 8;
	int screenX = 0,screenY = 0;
	for (int i=0;i<gNumX;i++)
	{
		for (int j=0;j<gNumY;j++)
		{
			screenX = oriX + gLength*i;
			screenY = oriY + gLength*j;
			clr = ::GetPixel(hDc,screenX,screenY);
			switch(clr)
			{
			case 0x0000FF:
				gBlock[i*gNumY+j] = 1;
			case 0x008000:
				gBlock[i*gNumY+j] = 2;
			case 0xFF0000:
				gBlock[i*gNumY+j] = 3;
			case 0x000080:
				gBlock[i*gNumY+j] = 4;
			case 0x800000:
				gBlock[i*gNumY+j] = 5;
			case 0x008080:
				gBlock[i*gNumY+j] = 6;
			case 0x000000:
				gBlock[i*gNumY+j] = 7;
			case 0x808080:
				gBlock[i*gNumY+j] = 8;
			case 0xC0C0C0:		//未点开或者是空白
				{
					clr = ::GetPixel(hDc,screenX-7,screenY-7);
					if(clr==0xFFFFFF)
						gBlock[i*gNumY+j] = -1;
					else
						gBlock[i*gNumY+j] = 0;
				}
			}
		}
	}
}

BOOL WINAPI setHook()
{
	BOOL result = FALSE;
	gOffset.x = 15-3;
	gOffset.y = 104-48;

	assert(gHHook==NULL);

	 gHandle = FindWindow(TEXT("扫雷"),NULL);
	 GetWindowRect(gHandle,&gRect);
	//get the target thread id
	DWORD dwThreadId = GetWindowThreadProcessId(gHandle,NULL);

	//get my hook program's thread id for message transfer
	gDwThreadId = GetCurrentThreadId();
//	gHHook = SetWindowsHookEx(WH_KEYBOARD,getMsgProc,gHinstDll,dwThreadId);

	result = (gHHook!=NULL);
//	if (result)
	{
		// post a message so that the hook function gets called
		//修改窗口标题
		LPCWSTR msg = TEXT("qq2015");
		result = SendMessage(gHandle,WM_SETTEXT,0,(LPARAM)msg);
		DWORD error = GetLastError();
		if(error)
			return FALSE;
		//发送鼠标消息
//		lParam = MAKELONG((gRect.left+gRect.right)/2,(gRect.top+gRect.bottom)/2);
		int x=0,y=0;
		HDC hDc = ::GetDC(NULL);
		while(1)
		{
			scanf("%d %d",&x,&y);
			if (x < 0 || x > 8 || y < 0 || y > 8)
			{
				::ReleaseDC(NULL, hDc);
				return FALSE;
			}
			char outString[100] = {0};
			COLORREF clr;
			int screenX=0,screenY=0;
			screenX -= 7;
			screenY -= 7;
			clr = ::GetPixel(hDc,screenX,screenY);
			sprintf(outString,"红：%d\n绿：%d\n蓝：%d",GetRValue(clr),GetGValue(clr),GetBValue(clr));
			cout << outString << endl;

			if(!clickBlock(x,y))
				return FALSE;
			updateMap(hDc);
			
// 			screenX = gOffset.x + 16*x + gRect.left + 3 + 8;
// 			screenY = gOffset.y + 16*y + gRect.top + 48 + 8;
// 			clr = ::GetPixel(hDc,screenX,screenY);
// 			
// 			sprintf(outString,"红：%d\n绿：%d\n蓝：%d",GetRValue(clr),GetGValue(clr),GetBValue(clr));
// 			cout << outString << endl;
		}

	}

	return result;
}

void unsetHook()
{
//	assert(gHHook!=NULL);
//	UnhookWindowsHookEx(gHHook);
	gHHook = NULL;
}

FILE* file = NULL;

LRESULT CALLBACK getMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
		//The DLL has been injected
	MessageBox(NULL,TEXT("@@@"),NULL,MB_OK);
//	SetWindowText(((LPCWPRETSTRUCT)lParam)->hwnd,TEXT("qq2012"));
		if (nCode == HC_ACTION)
		{
			//...process the target application
// 			if(wParam!=NULL)
// 			{
			
				switch(((LPMSG)lParam)->message)
				{
				case WM_SETTEXT:
					MessageBox(NULL,TEXT("@@@"),NULL,MB_OK);
					file = fopen(("D:\\1.txt"),"a+");
					fprintf(file,"%x\n",((LPMSG)lParam)->message);
					fclose(file);
//					SetWindowText(((LPMSG)lParam)->hwnd,(LPCWSTR)((LPMSG)lParam)->lParam);
					
					break;
				case WM_COMMAND:
					switch((UINT)HIWORD(wParam))
					{
					case  BN_CLICKED:
						
						Button_Enable((HWND)lParam,false);
						break;
					default:
						break;
					}
					break;
				default:
// 					if (bFirstTime)
// 					{
//						bFirstTime = FALSE;		
//						
//					}
					break;
				}		
//					SetWindowText(((LPMOUSEHOOKSTRUCT)lParam)->hwnd,TEXT("qq2012"));
//					PostThreadMessage(gDwThreadId, WM_NULL, 0, 0);
//			}
			
		}


	return CallNextHookEx(gHHook,nCode,wParam,lParam);
}