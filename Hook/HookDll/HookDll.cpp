// HookDll.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#include "stdafx.h"
#include <atlimage.h>
#include <WindowsX.h>
#include <stdio.h>
#include <iostream>
#include <strsafe.h>
#include <vector>


using namespace std;

//#define WIN7
#define XP

#define HOOKAPI  __declspec(dllexport)
#include "HookDll.h"

LRESULT WINAPI getMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
void ErrorExit(LPTSTR lpszFunction);
void gethash(HDC hDc, int x1, int y1, int x2, int y2,DWORD** clr);

#pragma data_seg("Shared")
DWORD gDwThreadId = 0;
HHOOK gHHook = NULL;
HWND gHandle = NULL;
RECT gRect;
POINT gOffset;
POINT gTitleOffset;
const int gNumX =16;
const int gNumY = 30;
const int gLength = 16;
#pragma data_seg()

#pragma comment(linker, "/section:Shared,rws")

HINSTANCE gHinstDll = NULL;

int gBlock[gNumX*gNumY];
HDC gHdcComp;
HGDIOBJ gHobjOrig;
HBITMAP gHbmp;

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
	int xpos = gOffset.x + 16*x - gTitleOffset.x + 8;
	int ypos = gOffset.y + 16*y - gTitleOffset.y + 8;
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
	int xpos = gOffset.x + 16*x - gTitleOffset.x + 8;
	int ypos = gOffset.y + 16*y - gTitleOffset.y + 8;
	LPARAM lParam = MAKELONG(xpos,ypos);
	SendMessage(gHandle,WM_RBUTTONDOWN,MK_RBUTTON,lParam);
	SendMessage(gHandle,WM_RBUTTONUP,0,lParam);
	gBlock[x*gNumY+y] = -2;
	return true;
}

bool doubleClickBlock(int x,int y)
{
	int xpos = gOffset.x + 16*x - gTitleOffset.x + 8;
	int ypos = gOffset.y + 16*y - gTitleOffset.y + 8;
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
	oriX = gOffset.x + gRect.left;
	oriY = gOffset.y + gRect.top;
#else
#ifdef WIN7
	oriX = gOffset.x + gRect.left;
	oriY = gOffset.y + gRect.top;
#endif
#endif
	DWORD* clr = NULL;
	gethash(hDc, oriX, oriY, oriX+gNumX*gLength-1, oriY+gNumY*gLength-1,&clr);

	int screenX = 0,screenY = 0;
	for (int i=0;i<gNumX;i++)
	{
		for (int j=0;j<gNumY;j++)
		{
			if(gBlock[i*gNumY+j] != -1)
				continue;
			switch(clr[((gNumY-j-1)*gLength+8)*gNumX*gLength+i*gLength+8] | 0xFF000000)
			{
			case 0xFF0000FF:
				gBlock[i*gNumY+j] = 1;
				break;
			case 0xFF008000:
				gBlock[i*gNumY+j] = 2;
				break;
			case 0xFFFF0000:
				if((clr[((gNumY-j-1)*gLength+6)*gNumX*gLength+i*gLength+8] | 0xFF000000)==0xFF000000)
					gBlock[i*gNumY+j] = -2;	//����
				else
					gBlock[i*gNumY+j] = 3;
				break;
			case 0xFF000080:
				gBlock[i*gNumY+j] = 4;
				break;
			case 0xFF800000:
				gBlock[i*gNumY+j] = 5;
				break;
			case 0xFF008080:
				gBlock[i*gNumY+j] = 6;
				break;
			case 0xFF000000:
				gBlock[i*gNumY+j] = 7;
				break;
			case 0xFF808080:
				gBlock[i*gNumY+j] = 8;
				break;
			case 0xFFC0C0C0:		//δ�㿪�����ǿհ�
				{
					if((clr[((gNumY-j-1)*gLength+15)*gNumX*gLength+i*gLength] | 0xFF000000)==0xFFFFFFFF)
						gBlock[i*gNumY+j] = -1;
					else
						gBlock[i*gNumY+j] = 0;
				}
				break;
			}
		}
	}
	SelectObject(gHdcComp, gHobjOrig);
	DeleteObject(gHbmp);
	DeleteDC(gHdcComp);
}

//����point��Χ8�����ӵ�״����������vector�У����ظõ���Χ��ʣ������
int searchAround(Block point,int num,vector<Block>& unOpen,vector<Block>& flag)
{
	int index = 0;
	if(point.x-1>=0)
	{
		//��ߵ�
		index = (point.x-1)*gNumY+point.y;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x-1,point.y));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x-1,point.y));
		
		if (point.y-1>=0)
		{
			//���ϵ�
			index = (point.x-1)*gNumY+point.y-1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x-1,point.y-1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x-1,point.y-1));
		}

		if (point.y+1<gNumY)
		{
			//���µ�
			index = (point.x-1)*gNumY+point.y+1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x-1,point.y+1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x-1,point.y+1));
		}
	}
	if (point.x+1 < gNumX)
	{
		//�ұߵ�
		index = (point.x+1)*gNumY+point.y;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x+1,point.y));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x+1,point.y));

		if (point.y-1>=0)
		{
			//���ϵ�
			index = (point.x+1)*gNumY+point.y-1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x+1,point.y-1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x+1,point.y-1));
		}

		if (point.y+1 < gNumY)
		{
			//���µ�
			index = (point.x+1)*gNumY+point.y+1;
			if(gBlock[index] == -2)
				flag.push_back(Block(point.x+1,point.y+1));
			else if(gBlock[index] == -1)
				unOpen.push_back(Block(point.x+1,point.y+1));
		}
	}
	if(point.y-1>=0)
	{
		//�ϵ�
		index = point.x*gNumY+point.y-1;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x,point.y-1));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x,point.y-1));
	}
	if (point.y+1<gNumY)
	{
		//�µ�
		index = point.x*gNumY+point.y+1;
		if(gBlock[index] == -2)
			flag.push_back(Block(point.x,point.y+1));
		else if(gBlock[index] == -1)
			unOpen.push_back(Block(point.x,point.y+1));
	}
	return (num-flag.size());
}


//��ʱ��һ������������Լ���ɨ�ף�����
// 2		1		2		|>
//	|>	2		4		|>
//	��	��	��	|>
//�������ϵ������4���жϵ㣬ʣ��1�ף�������2������δ����
//�������������������4���ھ�ͬ���Ӵ����������ӵĵ㣬Ҳ����2�����ж�
//����˸���ʣ������Ϊ1����������δ�㿪�ĸ��ӣ���ô�����������
//����˸���ʣ����������1���ҳ���4���ǿ����⣬ʣ���׵�����������δ�㿪��������ͬ��������������
bool checkDeep(Block point,vector<Block> unOpen,int result)
{
	//���unOpen�ǲ���ʣ��2��
	if(unOpen.size()!=2)
		return false;
	//ʣ����������Ϊ1��
	if (result != 1)
		return false;
	//2��������δ�㿪���ӣ���Դ����8��λ�ù�ϵ������һ����Դ��Ӵ�
	vector<Block> vTarUnOpen;
	vector<Block> vTarFlag;
	int tarResult = 0;
	for (int i=0;i<unOpen.size();i++)
	{
		tarResult = 0;
		vTarFlag.clear();
		vTarUnOpen.clear();
		//�Ӵ��������ұ�
		if((unOpen[i].x==point.x-1 && unOpen[i].y==point.y) ||
			(unOpen[i].x==point.x+1 && unOpen[i].y==point.y))
		{
			//��һ���ڲ���
			if(unOpen[1-i].x==unOpen[i].x &&unOpen[1-i].y==unOpen[i].y-1)
			{
				//��ʱ���Դ���Ϸ��ĵ�
				tarResult = searchAround(Block(point.x,point.y-1),gBlock[point.x*gNumY+point.y-1],vTarUnOpen,vTarFlag);
			}
			//��һ���ڲ���
			else  if (unOpen[1-i].x==unOpen[i].x && unOpen[1-i].y==unOpen[i].y+1)
			{
				//��ʱ���Դ���·��ĵ�
				tarResult = searchAround(Block(point.x,point.y+1),gBlock[point.x*gNumY+point.y+1],vTarUnOpen,vTarFlag);
			}
		}
		//�Ӵ��������±�
		else if ((unOpen[i].x==point.x && unOpen[i].y==point.y-1) ||
			(unOpen[i].x==point.x && unOpen[i].y==point.y+1))
		{
			//��һ���ڲ���
			if(unOpen[1-i].x==unOpen[i].x-1 &&unOpen[1-i].y==unOpen[i].y)
			{
				//��ʱ���Դ���󷽵ĵ�
				tarResult = searchAround(Block(point.x-1,point.y),gBlock[(point.x-1)*gNumY+point.y],vTarUnOpen,vTarFlag);
			}
			//��һ���ڲ���
			else  if (unOpen[1-i].x==unOpen[i].x+1 && unOpen[1-i].y==unOpen[i].y)
			{
				//��ʱ���Դ���ҷ��ĵ�
				tarResult = searchAround(Block(point.x+1,point.y),gBlock[(point.x+1)*gNumY+point.y],vTarUnOpen,vTarFlag);
			}
		}
		if(tarResult==1 && !vTarUnOpen.empty())
		{
			for (int j=0;j<vTarUnOpen.size();j++)
			{
				if((vTarUnOpen[j].x!=unOpen[0].x || vTarUnOpen[j].y!=unOpen[0].y) &&
					(vTarUnOpen[j].x!=unOpen[1].x || vTarUnOpen[j].y!=unOpen[1].y))
				{
					leftClickBlock(vTarUnOpen[j].x,vTarUnOpen[j].y);
					break;
				}
			}
		}
		else if(tarResult > 1 && (tarResult-1)==(vTarUnOpen.size()-2))
		{
			//Ŀ��������Դ�㹲�е�2��δ�㿪�����⣬����������
			for (int k=0;k<vTarUnOpen.size();k++)
			{
				if((vTarUnOpen[k].x!=unOpen[0].x || vTarUnOpen[k].y!=unOpen[0].y) &&
					(vTarUnOpen[k].x!=unOpen[1].x || vTarUnOpen[k].y!=unOpen[1].y))
				{
						rightClickBlock(vTarUnOpen[k].x,vTarUnOpen[k].y);
						break;
				}
			}
		}
	}
	return true;
}

bool checkFirst(HDC hDc)
{
	for (int i=0;i<gNumX;i++)
	{
		for (int j=0;j<gNumY;j++)
		{
			//���㿪�ĸ��ӣ��Ҳ��ǿհ�
			if(gBlock[i*gNumY+j] > 0)
			{
				vector<Block> vUnOpen;		//���������Χδ�㿪�ĸ���
				vector<Block> vFlag;			//���������Χ�������ӵĸ���
				int result = searchAround(Block(i,j),gBlock[i*gNumY+j],vUnOpen,vFlag);
				//��Χû��δ�㿪�ĸ���
				if(vUnOpen.size() == 0)
					continue;
				if(result==0)
				{
					//���Ѿ�����
					doubleClickBlock(i,j);
					updateMap(hDc);
				}
				else if (result > 0)
				{
					//δ�㿪��ȫ����
					if(vUnOpen.size() == result)
					{
						for (int k=0;k<vUnOpen.size();k++)
						{
							rightClickBlock(vUnOpen[k].x,vUnOpen[k].y);
						}
					}
					//��ʱδ�㿪�ĸ�����һ������ʣ������
					else
						checkDeep(Block(i,j),vUnOpen,result);
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
// 		scanf("%d %d",&x,&y);
// 		if (x < 0 || x >= gNumX || y < 0 || y >= gNumY)
// 		{
// 			::ReleaseDC(NULL, hDc);
// 			return;
// 		}
// 		char outString[100] = {0};
// 		COLORREF clr;
// 		int screenX=0,screenY=0;
// 		screenX -= 7;
// 		screenY -= 7;
// 		clr = ::GetPixel(hDc,screenX,screenY);
// 		sprintf(outString,"�죺%d\n�̣�%d\n����%d",GetRValue(clr),GetGValue(clr),GetBValue(clr));
// 		cout << outString << endl;

//		if(!leftClickBlock(x,y))
//			return;
		updateMap(hDc);
		checkFirst(hDc);
		// 			screenX = gOffset.x + 16*x + gRect.left + 3 + 8;
		// 			screenY = gOffset.y + 16*y + gRect.top + 48 + 8;
		// 			clr = ::GetPixel(hDc,screenX,screenY);
		// 			
		// 			sprintf(outString,"�죺%d\n�̣�%d\n����%d",GetRValue(clr),GetGValue(clr),GetBValue(clr));
		// 			cout << outString << endl;
	}
	::ReleaseDC(NULL, hDc);
}

BOOL WINAPI setHook()
{
	BOOL result = FALSE;
#ifdef XP
	gOffset.x = 15;
	gOffset.y = 104;
	gTitleOffset.x = 3;
	gTitleOffset.y = 48;
#else
#ifdef WIN7
	gOffset.x = 15;
	gOffset.y = 100;
	gTitleOffset.x = 4;
	gTitleOffset.y = 46;
#endif
#endif

	assert(gHHook==NULL);

	 gHandle = FindWindow(TEXT("ɨ��"),NULL);
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
		//�޸Ĵ��ڱ���
//		LPCWSTR msg = TEXT("qq2015");
//		result = SendMessage(gHandle,WM_SETTEXT,0,(LPARAM)msg);
//		ErrorExit(TEXT("SendMessage"));
		//���������Ϣ
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

void gethash(HDC hDc, int x1, int y1, int x2, int y2,DWORD** buffer)
{
//	COLORREF buffer[gNumX*gNumY*gLength*gLength];
// 	x1 = 100;
// 	y1 = 100;
// 	x2 = 200;
// 	y2 = 200;
//	COLORREF* buffer;
	//BGRA

	int cx, cy;
	BITMAPINFO bi;


	gHdcComp = CreateCompatibleDC(hDc);
//	ErrorExit(TEXT("CreateCompatibleDC"));
	if (!gHdcComp) {
		::ReleaseDC(NULL,hDc);
		return;
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

	gHbmp = CreateDIBSection(gHdcComp, &bi, DIB_RGB_COLORS, (void **)&(*buffer), NULL, 0);
	if (!gHbmp) {
		DeleteDC(gHdcComp);
		::ReleaseDC(NULL,hDc);
		return;
	}

	gHobjOrig = SelectObject(gHdcComp, gHbmp);
	if (!gHobjOrig) {
		DeleteObject(gHbmp);
		DeleteDC(gHdcComp);
		::ReleaseDC(NULL,hDc);
		return;
	}

	if (!BitBlt(gHdcComp, 0, 0, cx, cy, hDc, x1, y1, SRCCOPY)) {
		SelectObject(gHdcComp, gHobjOrig);
		DeleteObject(gHbmp);
		DeleteDC(gHdcComp);
		::ReleaseDC(NULL,hDc);
		return;
	}

// 	buffer[0] = 0;		//B
// 	buffer[1] = 0;		//G
// 	buffer[2] = 0xff;	//R
// 	buffer[3] = 0;		//A
// 
// 	buffer[4] = 0xff;
// 	buffer[5] = 0;
// 	buffer[6] = 0;
// 	buffer[7] = 0;
// 
// 	buffer[8] = 0;
// 	buffer[9] = 0xff;
// 	buffer[10] = 0;
// 	buffer[11] = 0;
// 	buffer[0] = 0xff0000;
// 	buffer[1] = 0xff;
// 	buffer[2] = 0xff00;
// 	int x = 2;
// 	int y = 3;
// 	buffer[((gNumY-y-1)*gLength+8)*gNumX*gLength+x*gLength+8] = 0xff0000;


// 	buffer[(gNumY-y)*gLength*gNumX*gLength+x*gLength] = 0;
// 	buffer[(gNumY-y)*gLength*gNumX*gLength+x*gLength+1] = 0;
// 	buffer[(gNumY-y)*gLength*gNumX*gLength+x*gLength+2] = 0xff;
// 	buffer[(gNumY-y)*gLength*gNumX*gLength+x*gLength+3] = 0;
// 	for (i = 0; i <  cy; ++i) {
// 	 	buffer[i] = 0;
// 		buffer[i+1] = 0;
// 		buffer[i+2] = 0xFF;
// 	}
// 	CImage image;
// 	image.Attach(gHbmp);
// 	image.Save(_T("c:\\B.bmp"));
// 	image.Detach();

//	::ReleaseDC(NULL,hDc);
}