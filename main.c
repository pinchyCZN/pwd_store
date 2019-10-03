#if _WIN32_WINNT<0x400
#define _WIN32_WINNT 0x400
#define WINVER 0x500
#endif
#if _WIN32_IE<=0x300
#define _WIN32_IE 0x400
#endif
#include <windows.h>
#include <Commctrl.h>
#include "resource.h"
#include "anchor_system.h"

HINSTANCE ghinstance=0;
HWND ghdlg=0;

struct CONTROL_ANCHOR anchor_main_dlg[]={
	{IDC_LISTVIEW,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_BOTTOM|ANCHOR_TOP,0,0,0},
	{IDC_STATIC_FL,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_STATIC_FR,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_FILTER_NAME,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_FILTER_DESC,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_ADD,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_EDIT,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_DELETE,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
};
#define _countof(x) sizeof(x)/sizeof(x[0])

int create_listview(HWND hwnd,int ctrl_id)
{
	HWND hlistview;
	HWND htmp;
	RECT rect;
	int w,h;
	htmp=GetDlgItem(hwnd,ctrl_id);
	if(0==htmp)
		return FALSE;
	GetWindowRect(hwnd,&rect);
	DestroyWindow(htmp);
	w=rect.right-rect.left;
	h=rect.bottom-rect.top;
	hlistview=CreateWindow(WC_LISTVIEW,"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
                                 0,0,
                                 w,h,
                                 hwnd,
                                 (HMENU)IDC_LISTVIEW,
                                 ghinstance,
                                 NULL);
	return TRUE;
}

BOOL CALLBACK dlg_func(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		create_listview(hwnd,IDC_LISTVIEW);
		AnchorInit(hwnd,anchor_main_dlg,_countof(anchor_main_dlg));
		break;
	case WM_SIZE:
		AnchorResize(hwnd,anchor_main_dlg,_countof(anchor_main_dlg));
		break;
	case WM_SIZING:
		{
			RECT dsize={0,0,300,300};
			RECT *size=(RECT*)lparam;
			if(size){
				return ClampMinWindowSize(&dsize,wparam,size);
			}
		}
		break;
	case WM_COMMAND:
		{
			int id=LOWORD(wparam);
			switch(id){
			case IDOK:
				break;
			case IDCANCEL:
				PostQuitMessage(0);
				break;
			}
		}
		break;
	}
	return FALSE;
}


int WINAPI WinMain(HINSTANCE hinstance,HINSTANCE hprev,LPSTR lpCmdLine,int nCmdShow)
{
	HWND hwnd;
	ghinstance=hinstance;
	hwnd=CreateDialog(ghinstance,MAKEINTRESOURCE(IDD_DIALOG),NULL,&dlg_func);
	if(0==hwnd){
		MessageBoxA(NULL,"Unable to create window","ERROR",MB_OK|MB_SYSTEMMODAL);
		return 0;
	}
	ShowWindow(hwnd,SW_SHOW);
	while(1){
		MSG msg;
		int res;
		res=GetMessage(&msg,NULL,0,0);
		if(0==res || -1==res){
			break;
		}
		if(!IsDialogMessage(hwnd,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
