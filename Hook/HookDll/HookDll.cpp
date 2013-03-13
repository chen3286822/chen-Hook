// HookDll.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <atlimage.h>
#include <WindowsX.h>
#include <stdio.h>
#include <iostream>
#include <strsafe.h>
#include <vector>


using namespace std;

#define WIN7
//#define XP

#define HOOKAPI  __declspec(dllexport)
#include "HookDll.h"

LRESULT WINAPI getMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
void ErrorExit(LPTSTR lpszFunction);
COLORREF* gethash(HDC hDc, int x1, int y1, int x2, int y2);

#pragma data_seg("Shared")
DWORD gDwThreadId = 0;
HHOOK gHHook = NULL;
HWND gHandle = NULL;
RECT gRect;
POINT gOffset;
const int gNumX = 16;
const int gNumY = 16;
const int gLength = 16;
#pragma data_seg()

#pragma comment(linker, "/section:Shared,rws")

HINSTANCE gHinstDll = NULL;

int gBlock[gNumX*gNumY];

struct Block
{
	int x;
	int y;
	Block(int _x,int _y)
	{
		x = _x;
		y = _y;
	}
};

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

bool leftClickBlock(int x,int y)
{
//	GetWindowRect(gHandle,&gRect);
	int xpos = gOffset.x + 16*x;
	int ypos = gOffset.y + 16*y;
	LPARAM lParam = MAKELONG(xpos,ypos);
	SendMessage(gHandle,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
//	if(GetLastError())
//		ErrorExit(TEXT("SendMessage"));
	SendMessage(gHandle,WM_LBUTTONUP,0,lParam);
//	if(GetLastError())
//		ErrorExit(TEXT("SendMessage"));
	return true;
}

bool rightClickBlock(int x,int y)
{

	int xpos = gOffset.x + 16*x;
	int ypos = gOffset.y + 16*y;
	LPARAM lParam = MAKELONG(xpos,ypos);
	SendMessage(gHandle,WM_RBUTTONDOWN,MK_RBUTTON,lParam);
	SendMessage(gHandle,WM_RBUTTONUP,0,lParam);
	return true;
}

bool doubleClickBlock(int x,int y)
{
	int xpos = gOffset.x + 16*x;
	int ypos = gOffset.y + 16*y;
	LPARAM lParam = MAKELONG(xpos,ypos);
	SendMessage(gHandle,WM_LBUTTONDOWN,MK_LBUTTON,lParam);
	SendMessage(gHandle,WM_RBUTTONDOWN,MK_LBUTTON|MK_RBUTTON,lParam);
	SendMessage(gHandle,WM_LBUTTONUP,MK_RBUTTON,lParam);
	SendMessage(gHandle,WM_RBUTTONUP,0,lParam);
	return true;
}

void updateMap(HDC hDc)
{
	int oriX = 0;
	int oriY = 0;
#ifdef XP
	oriX = gOffset.x + gRect.left + 3;
	oriY = gOffset.y + gRect.top + 48;
#else
#ifdef WIN7
	oriX = gOffset.x + gRect.left;
	oriY = gOffset.y + gRect.top;
#endif
#endif
	COLORREF* clr = gethash(hDc, oriX, oriY, oriX+gNumX*gLength, oriY+gNumY*gLength);
//	clr[1+1] = clr[2+2];
	/*
	COLORREF clr;

	int screenX = 0,screenY = 0;
	for (int i=0;i<gNumX;i++)
	{
		for (int j=0;j<gNumY;j++)
		{
			if(gBlock[i*gNumY+j] != -1)
				continue;
			screenX = oriX + gLength*i;
			screenY = oriY + gLength*j;
			clr = ::GetPixel(hDc,screenX,screenY);
			switch(clr)
			{
			case 0xff0000:
				gBlock[i*gNumY+j] = 1;
				break;
			case 0x008000:
				gBlock[i*gNumY+j] = 2;
				break;
			case 0x0000ff:
				gBlock[i*gNumY+j] = 3;
			case 0x800000:
				gBlock[i*gNumY+j] = 4;
				break;
			case 0x000080:
				gBlock[i*gNumY+j] = 5;
				break;
			case 0x808000:
				gBlock[i*gNumY+j] = 6;
				break;
			case 0x000000:
				{
					clr = ::GetPixel(hDc,screenX,screenY-2);
					if(clr==0x0000ff)
						gBlock[i*gNumY+j] = -2;	//红旗
					else
						gBlock[i*gNumY+j] = 7;
				}
				break;
			case 0x808080:
				gBlock[i*gNumY+j] = 8;
				break;
			case 0xC0C0C0:		//未点开或者是空白
				{
					clr = ::GetPixel(hDc,screenX-7,screenY-7);
					if(clr==0xFFFFFF)
						gBlock[i*gNumY+j] = -1;
					else
						gBlock[i*gNumY+j] = 0;
				}
				break;
			}
		}
	}*/
}

//检测点point周围8个格子的状况，保存在vector中，返回该点周围的剩余雷数
int searchAround(Block point,int num,vector<Block>& unOpen,vector<Block>& flag)
{
	int index = 0;
	if(point.x-1>=0)
	{
		//左边点
		index = (point.x-1)*gNumY+point.y;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x-1,point.y));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x-1,point.y));
		
		if (point.y-1>=0)
		{
			//左上点
			index = (point.x-1)*gNumY+point.y-1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x-1,point.y-1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x-1,point.y-1));
		}

		if (point.y+1<=8)
		{
			//左下点
			index = (point.x-1)*gNumY+point.y+1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x-1,point.y+1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x-1,point.y+1));
		}
	}
	if (point.x+1 <= 8)
	{
		//右边点
		index = (point.x+1)*gNumY+point.y;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x+1,point.y));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x+1,point.y));

		if (point.y-1>=0)
		{
			//右上点
			index = (point.x+1)*gNumY+point.y-1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x+1,point.y-1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x+1,point.y-1));
		}

		if (point.y+1 <= 8)
		{
			//右下点
			index = (point.x+1)*gNumY+point.y+1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x+1,point.y+1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x+1,point.y+1));
		}
	}
	if(point.y-1>=0)
	{
		//上点
		index = point.x*gNumY+point.y-1;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x,point.y-1));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x,point.y-1));
	}
	if (point.y+1<=8)
	{
		//下点
		index = point.x*gNumY+point.y+1;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x,point.y+1));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x,point.y+1));
	}
	return (num-flag.size());
}

bool checkFirst(HDC hDc)
{
	for (int i=0;i<gNumX;i++)
	{
		for (int j=0;j<gNumY;j++)
		{
			//检查点开的格子，且不是空白
			if(gBlock[i*gNumY+j] > 0)
			{
				vector<Block> vUnOpen;		//保存监测点周围未点开的格子
				vector<Block> vFlag;			//保存监测点周围插上旗子的格子
				int result = searchAround(Block(i,j),gBlock[i*gNumY+j],vUnOpen,vFlag);
				//周围没有未点开的格子
				if(vUnOpen.size() == 0)
					continue;
				if(result==0)
				{
					//雷已经排完
					doubleClickBlock(i,j);
					updateMap(hDc);
				}
				else if (result > 0)
				{
					//未点开的全是雷
					if(vUnOpen.size() == result)
					{
						for (int k=0;k<vUnOpen.size();k++)
						{
							rightClickBlock(vUnOpen[k].x,vUnOpen[k].y);
							gBlock[vUnOpen[k].x*gNumY+vUnOpen[k].y] = -2;
						}
					}

					//此时未点开的格子数一定大于剩余雷数
					//此时有一种特殊情况可以继续扫雷，如下
					// 2		1		2		|>
					//	|>		2		4		|>
					//	口	口	口	|>
					//类似以上的情况，4是判断点，剩余1雷，但是有2个格子未翻开
					//这两个格子相连，则对4的邻居同样接触这两个格子的点，也就是2进行判断
					//如果此格子剩余雷数为1，且有其他未点开的格子，那么点击其他格子
					//如果此格子剩余雷数大于1，且除开4的那颗雷外，剩余雷的数量与其他未点开格子数相同，则标记其他格子
				}
			}
		}
	}
	return true;
}


void sweepMine()
{
	memset(gBlock,-1,gNumX*gNumY*sizeof(int));
	int x=0,y=0;
	HDC hDc = ::GetDC(NULL);
	while(1)
	{
		scanf("%d %d",&x,&y);
		if (x < 0 || x >= gNumX || y < 0 || y >= gNumY)
		{
			::ReleaseDC(NULL, hDc);
			return;
		}
// 		char outString[100] = {0};
// 		COLORREF clr;
// 		int screenX=0,screenY=0;
// 		screenX -= 7;
// 		screenY -= 7;
// 		clr = ::GetPixel(hDc,screenX,screenY);
// 		sprintf(outString,"红：%d\n绿：%d\n蓝：%d",GetRValue(clr),GetGValue(clr),GetBValue(clr));
// 		cout << outString << endl;

//		if(!leftClickBlock(x,y))
//			return;
		updateMap(hDc);
		checkFirst(hDc);
		// 			screenX = gOffset.x + 16*x + gRect.left + 3 + 8;
		// 			screenY = gOffset.y + 16*y + gRect.top + 48 + 8;
		// 			clr = ::GetPixel(hDc,screenX,screenY);
		// 			
		// 			sprintf(outString,"红：%d\n绿：%d\n蓝：%d",GetRValue(clr),GetGValue(clr),GetBValue(clr));
		// 			cout << outString << endl;
	}
}

BOOL WINAPI setHook()
{
	BOOL result = FALSE;
#ifdef XP
	gOffset.x = 15-3;
	gOffset.y = 104-48;
#else
#ifdef WIN7
	gOffset.x = 16;
	gOffset.y = 101;
#endif
#endif

	assert(gHHook==NULL);

	 gHandle = FindWindow(TEXT("扫雷"),NULL);
//	gHandle = FindWindow(TEXT("Minesweeper"),NULL);
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
//		LPCWSTR msg = TEXT("qq2015");
//		result = SendMessage(gHandle,WM_SETTEXT,0,(LPARAM)msg);
//		ErrorExit(TEXT("SendMessage"));
		//发送鼠标消息
//		lParam = MAKELONG((gRect.left+gRect.right)/2,(gRect.top+gRect.bottom)/2);
		
		sweepMine();
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


void ErrorExit(LPTSTR lpszFunction) 
{ 
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw); 
}

COLORREF * gethash(HDC hDc, int x1, int y1, int x2, int y2)
{
//	COLORREF buffer[gNumX*gNumY*gLength*gLength];
// 	x1 = 100;
// 	y1 = 100;
// 	x2 = 200;
// 	y2 = 200;
//	COLORREF* buffer;
	BYTE* buffer;
	HDC hdcComp;
	HGDIOBJ hobjOrig;
	HBITMAP hbmp;
	int cx, cy;
	BITMAPINFO bi;
	int i;


	hdcComp = CreateCompatibleDC(hDc);
//	ErrorExit(TEXT("CreateCompatibleDC"));
	if (!hdcComp) {
		::ReleaseDC(NULL,hDc);
		return NULL;
	}

	cx = x2 - x1 + 1;
	cy = y2 - y1 + 1;

	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = cx;
	bi.bmiHeader.biHeight = cy;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = cx * cy * 4;
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;

	hbmp = CreateDIBSection(hdcComp, &bi, DIB_RGB_COLORS, (void **)&buffer, NULL, 0);
	if (!hbmp) {
		DeleteDC(hdcComp);
		::ReleaseDC(NULL,hDc);
		return NULL;
	}

	hobjOrig = SelectObject(hdcComp, hbmp);
	if (!hobjOrig) {
		DeleteObject(hbmp);
		DeleteDC(hdcComp);
		::ReleaseDC(NULL,hDc);
		return NULL;
	}

	if (!BitBlt(hdcComp, 0, 0, cx, cy, hDc, x1, y1, SRCCOPY)) {
		SelectObject(hdcComp, hobjOrig);
		DeleteObject(hbmp);
		DeleteDC(hdcComp);
		::ReleaseDC(NULL,hDc);
		return NULL;
	}

	buffer[0] = 0;		//G
	buffer[1] = 0;		//B
	buffer[2] = 0xff;	//R
	buffer[3] = 0;		//A

	buffer[4] = 0xff;
	buffer[5] = 0;
	buffer[6] = 0;
	buffer[7] = 0;

	buffer[8] = 0;
	buffer[9] = 0xff;
	buffer[10] = 0;
	buffer[11] = 0;
// 	for (i = 0; i <  cy; ++i) {
// 	 	buffer[i] = 0;
// 		buffer[i+1] = 0;
// 		buffer[i+2] = 0xFF;
// 	}
	CImage image;
	image.Attach(hbmp);
	image.Save(_T("c:\\B.bmp"));
	image.Detach();


	SelectObject(hdcComp, hobjOrig);
	DeleteObject(hbmp);
	DeleteDC(hdcComp);
//	::ReleaseDC(NULL,hDc);
	return NULL;
}