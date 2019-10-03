#define UNICODE
#if _WIN32_WINNT<0x400
#define _WIN32_WINNT 0x400
#define WINVER 0x500
#endif
#if _WIN32_IE<=0x300
#define _WIN32_IE 0x400
#endif
#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include "resource.h"
#include "anchor_system.h"
#define _countof(x) sizeof(x)/sizeof(x[0])

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
	{IDC_GRIPPY,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
};

WCHAR *col_names[]={
	L"USER",L"PASSWORD",L"DESCRIPTION"
};
typedef struct{
	WCHAR *user;
	WCHAR *pwd;
	WCHAR *desc;
}PWD_ENTRY;

typedef struct{
	PWD_ENTRY *list;
	int count;
}PWD_LIST;

PWD_LIST g_pwd_list={0};

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
	hlistview=CreateWindow(WC_LISTVIEW,L"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
                                 0,0,
                                 w,h,
                                 hwnd,
                                 (HMENU)IDC_LISTVIEW,
                                 ghinstance,
                                 NULL);
	return TRUE;
}
int get_first_line_len(char *str)
{
	int i,len;
	len=0x10000;
	for(i=0;i<len;i++){
		if(str[i]=='\n' || str[i]=='\r' || str[i]==0)
			break;
	}
	return i;
}
int get_string_width_wc(HWND hwnd,void *str,int wide_char)
{
	if(hwnd!=0 && str!=0){
		SIZE size={0};
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			HFONT hfont;
			int len=get_first_line_len(str);
			hfont=(HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
			if(hfont!=0){
				HGDIOBJ hold=0;
				hold=SelectObject(hdc,hfont);
				if(wide_char)
					GetTextExtentPoint32W(hdc,(LPCWSTR)str,wcslen((WCHAR*)str),&size);
				else
					GetTextExtentPoint32A(hdc,str,len,&size);
				if(hold!=0)
					SelectObject(hdc,hold);
			}
			else{
				if(wide_char)
					GetTextExtentPoint32W(hdc,(LPCWSTR)str,wcslen((WCHAR*)str),&size);
				else
					GetTextExtentPoint32A(hdc,str,len,&size);
			}
			ReleaseDC(hwnd,hdc);
			return size.cx;
		}
	}
	return 0;

}
int get_str_width(HWND hwnd,void *str)
{
	return get_string_width_wc(hwnd,str,FALSE);
}
int setup_listview(HWND hlist,WCHAR **cols,int count)
{
	int i;
	HWND header=ListView_GetHeader(hlist);
	if(0==header)
		header=hlist;
	for(i=0;i<count;i++){
		LV_COLUMNW col={0};
		WCHAR *tmp=cols[i];
		col.mask=LVCF_TEXT|LVCF_WIDTH;
		col.fmt=LVCFMT_LEFT;
		col.pszText=tmp;
		col.iSubItem=0;
		col.cx=get_str_width(header,tmp)+12;
		ListView_InsertColumn(hlist,i,&col);
	}
	return 0;
}
char *wchar2utf(const WCHAR *str)
{
	char *result=0;
	char *tmp=0;
	int len,tmp_size;
	len=WideCharToMultiByte(CP_UTF8,0,str,-1,NULL,0,NULL,NULL);
	if(0==len)
		return result;
	tmp_size=len;
	tmp=calloc(tmp_size,1);
	if(tmp){
		int res;
		res=WideCharToMultiByte(CP_UTF8,0,str,-1,tmp,tmp_size,NULL,NULL);
		if(res==len){
			tmp[tmp_size-1]=0;
			result=tmp;
		}else{
			free(tmp);
		}
	}
	return result;
}

WCHAR *utf2wchar(const char *str)
{
	WCHAR *result=0;
	WCHAR *tmp;
	int tmp_size;
	int count;
	count=MultiByteToWideChar(CP_UTF8,0,str,-1,NULL,0);
	if(0==count)
		return result;
	tmp_size=count*sizeof(WCHAR);
	tmp=calloc(tmp_size,1);
	if(tmp){
		int res;
		res=MultiByteToWideChar(CP_UTF8,0,str,-1,tmp,count);
		if(res==count){
			tmp[count-1]=0;
			result=tmp;
		}else{
			free(tmp);
		}
	}
	return result;
}
const WCHAR *get_ini_fname()
{
	static WCHAR path[2048]={0};
	static int init=FALSE;
	if(!init){
		int res;
		const WCHAR *delim=L"\\";
		memset(path,0,sizeof(path));
		res=GetModuleFileNameW(NULL,path,_countof(path));
		if(res){
			int i,plen=wcslen(path);
			for(i=plen-1;i>=0;i--){
				WCHAR a;
				a=path[i];
				if(L'\\'==a){
					path[i]=0;
					break;
				}
			}
			init=TRUE;
		}
		if(0==path[0])
			delim=L"";
		_snwprintf(path,_countof(path),L"%s%s%s",path,delim,L"PASSWORD_LIST.INI");
	}
	return path;
}
int get_ini_str(const WCHAR *section,const WCHAR *key,WCHAR **out)
{
	int result=FALSE;
	WCHAR *tmp;
	int tmp_size=4096;
	tmp=calloc(tmp_size,1);
	if(tmp){
		int count=tmp_size/sizeof(WCHAR);
		GetPrivateProfileStringW(section,key,L"",tmp,count,get_ini_fname());
		if(tmp[0]){
			*out=wcsdup(tmp);
			result=TRUE;
		}
		free(tmp);
	}
	return result;
}
int get_ini_val(const WCHAR *section,const WCHAR *key,int *val)
{
	WCHAR *tmp=0;
	get_ini_str(section,key,&tmp);
	if(tmp){
		val[0]=_wtoi(tmp);
		free(tmp);
		return TRUE;
	}
	return FALSE;
}
void free_pwd_list(PWD_LIST *plist)
{
	int i,count;
	count=plist->count;
	for(i=0;i<count;i++){
		PWD_ENTRY *entry;
		entry=&plist->list[i];
		free(entry->desc);
		free(entry->pwd);
		free(entry->user);
	}
	free(plist->list);
	memset(plist,0,sizeof(PWD_LIST));
}
int add_pwd(PWD_LIST *list,WCHAR *user,WCHAR *pwd,WCHAR *desc)
{
	int result=FALSE;
	int index,count,size;
	PWD_ENTRY *entry;
	index=count=list->count;
	count++;
	size=sizeof(PWD_ENTRY)*count;
	entry=realloc(list->list,size);
	if(entry){
		list->list=entry;
		entry=&entry[index];
		if(0==desc)
			desc=L"";
		if(0==pwd)
			pwd=L"";
		if(0==user)
			user=L"";
		entry->desc=wcsdup(desc);
		entry->pwd=wcsdup(pwd);
		entry->user=wcsdup(user);
		list->count=count;
		result=TRUE;
	}
	return result;
}
int get_pwd_list(PWD_LIST *plist)
{
	int i,count;
	free_pwd_list(plist);
	count=0;
	get_ini_val(L"SETTINGS",L"ENTRY_COUNT",&count);
	for(i=0;i<count;i++){
		WCHAR section[80]={0};
		WCHAR *user=0,*pwd=0,*desc=0;
		_snwprintf(section,_countof(section),L"ENTRY%i",i);
		get_ini_str(section,L"USER",&user);
		get_ini_str(section,L"PWD",&pwd);
		get_ini_str(section,L"DESC",&desc);
		if(user||pwd||desc){
			add_pwd(plist,user,pwd,desc);
		}
		free(user);
		free(pwd);
		free(desc);
	}
	return TRUE;
}
int populate_listview(HWND hlist,PWD_LIST *plist)
{
	int i,count;
	int max_width=0;
	int widths[3]={0};
	HWND header=ListView_GetHeader(hlist);
	count=plist->count;
	ListView_DeleteAllItems(hlist);
	for(i=0;i<count;i++){
		LV_ITEMW item={0};
		PWD_ENTRY *entry=&plist->list[i];
		WCHAR *tmp;
		int *val;
		int x;
		tmp=entry->user;
		val=&widths[0];
		item.mask=LVIF_TEXT;
		item.iItem=i;
		item.iSubItem=0;
		item.pszText=tmp;
		ListView_InsertItem(hlist,&item);
		x=get_string_width_wc(header,tmp,0);
		if(x>=val[0])
			val[0]=x;
		//---
		tmp=entry->pwd;
		val=&widths[1];
		item.iSubItem=1;
		item.pszText=tmp;
		ListView_SetItem(hlist,&item);
		x=get_string_width_wc(header,tmp,0);
		if(x>=val[0])
			val[0]=x;
		//---
		tmp=entry->desc;
		val=&widths[2];
		item.iSubItem=2;
		item.pszText=tmp;
		ListView_SetItem(hlist,&item);
		x=get_string_width_wc(header,tmp,0);
		if(x>=val[0])
			val[0]=x;
	}
	count=_countof(widths);
	for(i=0;i<count;i++){
		int x=get_string_width_wc(header,col_names[i],0);
		if(x>widths[i])
			widths[i]=x;
	}
	count=_countof(widths);
	{
		RECT rect={0};
		int w;
		GetWindowRect(hlist,&rect);
		w=rect.right-rect.left;
		if(w>0)
			max_width=w;
	}
	for(i=0;i<count;i++){
		int x=widths[i]+12;
		if(max_width && x>max_width)
			x=max_width;
		ListView_SetColumnWidth(hlist,i,x);
	}
	return TRUE;
}



BOOL CALLBACK dlg_func(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		{
			HWND hlist;
			int style;
			HWND hgrip=GetDlgItem(hwnd,IDC_GRIPPY);
			style=GetWindowLong(hgrip,GWL_STYLE);
			style|=SBS_SIZEGRIP;
			SetWindowLong(hgrip,GWL_STYLE,style);
			create_listview(hwnd,IDC_LISTVIEW);
			setup_listview(GetDlgItem(hwnd,IDC_LISTVIEW),col_names,_countof(col_names));
			AnchorInit(hwnd,anchor_main_dlg,_countof(anchor_main_dlg));
			get_pwd_list(&g_pwd_list);
			hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
			populate_listview(hlist,&g_pwd_list);
		}
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
