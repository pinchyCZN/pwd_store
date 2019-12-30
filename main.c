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

int show_pwd_dlg(HWND,HWND,HINSTANCE);

HINSTANCE ghinstance=0;
HWND ghdlg=0;

struct CONTROL_ANCHOR anchor_main_dlg[]={
	{IDC_LISTVIEW,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_BOTTOM|ANCHOR_TOP,0,0,0},
	{IDC_STATIC_FR,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_FILTER_DESC,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_ADD,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_EDIT,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_DELETE,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_ON_TOP,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_GRIPPY,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_SAVE,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_PWD_GEN,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
};

WCHAR *col_names[]={
	L"USER",L"PASSWORD",L"DESCRIPTION"
};
#define SECTION_WINDOW L"WINDOW"
#define KEY_XPOS L"XPOS"
#define KEY_YPOS L"YPOS"

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
PWD_LIST g_first_pwd_list={0};

int create_listview(HWND hwnd,int ctrl_id)
{
	HWND hlistview;
	HWND htmp;
	RECT rect;
	int w,h;
	htmp=GetDlgItem(hwnd,ctrl_id);
	if(0==htmp)
		return FALSE;
	GetWindowRect(htmp,&rect);
	DestroyWindow(htmp);
	w=rect.right-rect.left;
	h=rect.bottom-rect.top;
	hlistview=CreateWindow(WC_LISTVIEW,L"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS|LVS_OWNERDRAWFIXED,
                                 0,0,
                                 w,h,
                                 hwnd,
                                 (HMENU)IDC_LISTVIEW,
                                 ghinstance,
                                 NULL);
	if(hlistview)
		ListView_SetExtendedListViewStyle(hlistview,LVS_EX_FULLROWSELECT);
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
		col.cx=get_string_width_wc(header,tmp,1)+12;
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
int set_ini_str(const WCHAR *section,const WCHAR *key,WCHAR *val)
{
	return WritePrivateProfileStringW(section,key,val,get_ini_fname());
}
int set_ini_val(const WCHAR *section,const WCHAR *key,int val)
{
	WCHAR str[80]={0};
	_snwprintf(str,_countof(str),L"%u",val);
	return WritePrivateProfileStringW(section,key,str,get_ini_fname());
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
int add_pwd_entry(PWD_LIST *list,WCHAR *user,WCHAR *pwd,WCHAR *desc)
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
int del_pwd_entry(PWD_LIST *list,const int index)
{
	int result=FALSE;
	PWD_ENTRY *entry;
	int count,size;
	if(index<0 || index>=list->count)
		return result;
	count=list->count-1;
	size=sizeof(PWD_ENTRY)*count;
	entry=calloc(size,1);
	if(entry){
		int i,new_index=0;
		for(i=0;i<list->count;i++){
			if(index!=i){
				entry[new_index++]=list->list[i];
			}
		}
		result=TRUE;
		free(list->list);
		list->list=entry;
		list->count=count;
	}
	return result;	
}
int update_pwd_entry(PWD_LIST *plist,int index,const WCHAR *user,const WCHAR *pwd,const WCHAR *desc)
{
	int result=FALSE;
	PWD_ENTRY *pentry;
	if(index<0 || index>=plist->count)
		return result;
	pentry=&plist->list[index];
	free(pentry->desc);
	free(pentry->pwd);
	free(pentry->user);
	pentry->desc=wcsdup(desc);
	pentry->pwd=wcsdup(pwd);
	pentry->user=wcsdup(user);
	result=TRUE;
	return result;
}
int load_pwd_list(PWD_LIST *plist)
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
			add_pwd_entry(plist,user,pwd,desc);
		}
		free(user);
		free(pwd);
		free(desc);
	}
	return TRUE;
}
int save_pwd_list(PWD_LIST *plist)
{
	int i,count;
	count=plist->count;
	set_ini_val(L"SETTINGS",L"ENTRY_COUNT",count);
	for(i=0;i<count;i++){
		WCHAR section[80]={0};
		PWD_ENTRY *entry=&plist->list[i];
		_snwprintf(section,_countof(section),L"ENTRY%i",i);
		set_ini_str(section,L"USER",entry->user);
		set_ini_str(section,L"PWD",entry->pwd);
		set_ini_str(section,L"DESC",entry->desc);
	}
	return TRUE;
}
int compare_pwdlist(PWD_LIST *p1,PWD_LIST *p2)
{
	int result=0;
	int i,count;
	result=p1->count-p2->count;
	if(result)
		return result;
	count=p1->count;
	for(i=0;i<count;i++){
		int x;
		PWD_ENTRY *e1,*e2;
		e1=&p1->list[i];
		e2=&p2->list[i];
		x=wcscmp(e1->user,e2->user);
		if(x)
			return x;
		x=wcscmp(e1->pwd,e2->pwd);
		if(x)
			return x;
		x=wcscmp(e1->desc,e2->desc);
		if(x)
			return x;
	}
	return result;
}
int populate_listview(HWND hlist,PWD_LIST *plist)
{
	int i,count,index=0;
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
		if(0==entry->user)
			continue;
		tmp=entry->user;
		val=&widths[0];
		item.mask=LVIF_TEXT|LVIF_PARAM;
		item.iItem=index++;
		item.iSubItem=0;
		item.pszText=tmp;
		item.lParam=i;
		ListView_InsertItem(hlist,&item);
		x=get_string_width_wc(header,tmp,1);
		if(x>=val[0])
			val[0]=x;
		//---
		tmp=entry->pwd;
		val=&widths[1];
		item.mask=LVIF_TEXT;
		item.iSubItem=1;
		item.pszText=tmp;
		ListView_SetItem(hlist,&item);
		x=get_string_width_wc(header,tmp,1);
		if(x>=val[0])
			val[0]=x;
		//---
		tmp=entry->desc;
		val=&widths[2];
		item.iSubItem=2;
		item.pszText=tmp;
		ListView_SetItem(hlist,&item);
		x=get_string_width_wc(header,tmp,1);
		if(x>=val[0])
			val[0]=x;
	}
	count=_countof(widths);
	for(i=0;i<count;i++){
		int x=get_string_width_wc(header,col_names[i],1);
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
int get_focused_item(HWND hlist)
{
	int result=-1;
	int i,count;
	if(0==hlist)
		return result;
	count=ListView_GetItemCount(hlist);
	for(i=0;i<count;i++){
		int state=ListView_GetItemState(hlist,i,LVIS_FOCUSED);
		if(LVIS_FOCUSED==state){
			result=i;
			break;
		}
	}
	return result;
}
int get_entry_text(HWND hlist,int index,WCHAR **user,WCHAR **pwd,WCHAR **desc)
{
	int result=FALSE;
	WCHAR *tmp=0;
	int tmp_size=4096;
	int tmp_count=tmp_size/sizeof(WCHAR);
	LV_ITEM item={0};
	tmp=malloc(tmp_size);
	if(0==tmp){
		return result;
	}
	memset(tmp,0,tmp_size);
	item.mask=LVIF_TEXT;
	item.iItem=index;
	item.pszText=tmp;
	item.cchTextMax=tmp_count;
	ListView_GetItem(hlist,&item);
	*user=wcsdup(tmp);
	memset(tmp,0,tmp_size);
	item.mask=LVIF_TEXT;
	item.iItem=index;
	item.pszText=tmp;
	item.cchTextMax=tmp_count;
	item.iSubItem=1;
	ListView_GetItem(hlist,&item);
	*pwd=wcsdup(tmp);
	memset(tmp,0,tmp_size);
	item.mask=LVIF_TEXT;
	item.iItem=index;
	item.pszText=tmp;
	item.cchTextMax=tmp_count;
	item.iSubItem=2;
	ListView_GetItem(hlist,&item);
	*desc=wcsdup(tmp);
	if(user[0] && pwd[0] && desc[0])
		result=TRUE;
	free(tmp);
	return result;
}
int get_entry_lparam(HWND hlist,int index,int *out)
{
	int result=FALSE;
	LV_ITEM item={0};
	int res;
	item.mask=LVIF_PARAM;
	item.iItem=index;
	res=ListView_GetItem(hlist,&item);
	if(res){
		*out=item.lParam;
		result=TRUE;
	}
	return result;
}

int set_entry_text(HWND hlist,int index,WCHAR *user,WCHAR *pwd,WCHAR *desc)
{
	ListView_SetItemText(hlist,index,0,user);
	ListView_SetItemText(hlist,index,1,pwd);
	ListView_SetItemText(hlist,index,2,desc);
	return TRUE;
}
int add_entry_text(HWND hlist,int lparam_val,WCHAR *user,WCHAR *pwd,WCHAR *desc)
{
	int index,count;
	LV_ITEM item={0};
	count=ListView_GetItemCount(hlist);
	item.mask=LVIF_TEXT|LVIF_PARAM;
	item.iItem=count;
	item.pszText=user;
	item.lParam=lparam_val;
	index=ListView_InsertItem(hlist,&item);
	if(index>=0){
		int state=LVIS_SELECTED|LVIS_FOCUSED;
		item.mask=LVIF_TEXT;
		item.iItem=count;
		item.iSubItem=1;
		item.pszText=pwd;
		ListView_SetItem(hlist,&item);
		item.mask=LVIF_TEXT;
		item.iItem=count;
		item.iSubItem=2;
		item.pszText=desc;
		ListView_SetItem(hlist,&item);
		ListView_SetItemState(hlist,index,state,state);
		return TRUE;
	}
	return FALSE;
}
int get_hwnd_wchar(HWND hwnd,WCHAR **out)
{
	WCHAR *tmp;
	int tmp_size=4096;
	int tmp_count=tmp_size/sizeof(WCHAR);
	tmp=calloc(tmp_size,1);
	if(0==tmp)
		return FALSE;
	GetWindowTextW(hwnd,tmp,tmp_count);
	*out=wcsdup(tmp);
	free(tmp);
	return TRUE;
}
int auto_set_list_width(HWND hlist)
{
	WCHAR *tmp;
	int tmp_size=4096;
	int tmp_count=tmp_size/sizeof(WCHAR);
	int i,count;
	int widths[3]={0};
	tmp=malloc(tmp_size);
	if(0==tmp)
		return FALSE;
	count=ListView_GetItemCount(hlist);
	for(i=0;i<count;i++){
		int j;
		for(j=0;j<_countof(widths);j++){
			LV_ITEM item={0};
			int x;
			memset(tmp,0,tmp_size);
			item.mask=LVIF_TEXT;
			item.iItem=i;
			item.iSubItem=j;
			item.pszText=tmp;
			item.cchTextMax=tmp_count;
			ListView_GetItem(hlist,&item);
			x=get_string_width_wc(hlist,tmp,1);
			if(x>widths[j])
				widths[j]=x;
		}
	}
	for(i=0;i<_countof(widths);i++){
		LV_COLUMN col={0};
		WCHAR str[80]={0};
		int x;
		col.mask=LVCF_TEXT;
		col.pszText=str;
		col.cchTextMax=_countof(str);
		ListView_GetColumn(hlist,i,&col);
		x=get_string_width_wc(hlist,str,1);
		if(x>widths[i])
			widths[i]=x;
	}
	for(i=0;i<_countof(widths);i++){
		int x=widths[i];
		if(x){
			x+=12;
			ListView_SetColumnWidth(hlist,i,x);
		}
	}
	free(tmp);
	return TRUE;
}

WNDPROC orig_edit_proc=0;
BOOL CALLBACK edit_subclass(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_KEYDOWN:
		{
			int key=wparam;
			if('A'==key){
				int ctrl=GetKeyState(VK_CONTROL)&0x8000;
				if(ctrl){
					SendMessage(hwnd,EM_SETSEL,0,-1);
					return 0;
				}
			}
		}
		break;
	}
	return CallWindowProc(orig_edit_proc,hwnd,msg,wparam,lparam);
}
int setup_edit_controls(HWND hwnd,int *list,int count)
{
	int i;
	for(i=0;i<count;i++){
		HWND hedit;
		hedit=GetDlgItem(hwnd,list[i]);
		if(hedit){
			void *ptr;
			SendMessage(hedit,EM_SETSEL,-1,-1);
			ptr=(void*)GetWindowLong(hedit,GWL_WNDPROC);
			if(ptr){
				orig_edit_proc=ptr;
				SetWindowLong(hedit,GWL_WNDPROC,(LONG)&edit_subclass);
			}
		}
	}
	return TRUE;
}
int is_valid_entry(const WCHAR *a,const WCHAR *b,const WCHAR *c)
{
	int result=FALSE;
	if(wcslen(a)>0)
		result=TRUE;
	if(wcslen(b)>0)
		result=TRUE;
	if(wcslen(c)>0)
		result=TRUE;
	return result;
}
void handle_paste_str(HWND hlist,WCHAR *str)
{
	int i,len,line_count=0;
	len=wcslen(str);
	for(i=0;i<len;i++){
		WCHAR a=str[i];
		if(L'\n'==a){
			line_count++;
		}
	}
	if(0==line_count)
		return;

}
WCHAR *get_clipboard()
{
	WCHAR *result=0;
	int res;
	HANDLE hclip;
	res=OpenClipboard(NULL);
	if(!res)
		return result;
	hclip=GetClipboardData(CF_UNICODETEXT);
	if(hclip){
		WCHAR *str=GlobalLock(hclip);
		if(str){
			result=wcsdup(str);
			GlobalUnlock(hclip);
		}
	}
	CloseClipboard();
	return result;
}
int copy_to_clip(WCHAR *str)
{
	int len,size,result=FALSE;
	HGLOBAL hmem;
	len=wcslen(str);
	if(len==0)
		return result;
	size=(len+1)*sizeof(WCHAR);
	hmem=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,size);
	if(hmem!=0){
		WCHAR *lock;
		lock=GlobalLock(hmem);
		if(lock!=0){
			memcpy(lock,str,size);
			GlobalUnlock(hmem);
			if(OpenClipboard(NULL)!=0){
				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT,hmem);
				CloseClipboard();
				result=TRUE;
			}
		}
		if(!result)
			GlobalFree(hmem);
	}
	return result;
}
void free_split_words(WCHAR ***list,int *count)
{
	int i,__count=*count;
	WCHAR **__list=*list;
	for(i=0;i<__count;i++){
		free(__list[i]);
	}
	free(__list);
	*list=0;
	*count=0;
}
int split_words(WCHAR *str,WCHAR ***out,int *out_count)
{
	int index=0;
	WCHAR *start=str;
	int state=0;
	WCHAR **list=0;
	int list_count=0;
	while(1){
		int is_end=FALSE;
		int parse=FALSE;
		WCHAR a=str[index];
		if(0==a || ' '==a){
			if(0==state){
				parse=TRUE;
				state=1;
			}
		}else{
			if(1==state)
				start=&str[index];
			state=0;
		}
		if(0==a)
			is_end=TRUE;
		if(parse){
			SIZE_T len,x1,x2;
			x1=(SIZE_T)start;
			x2=(SIZE_T)(&str[index]);
			len=x2-x1;
			if(len){
				int size,list_index;
				WCHAR **tmp;
				size=(list_count+1)*sizeof(WCHAR*);
				list_index=list_count;
				tmp=realloc(list,size);
				if(tmp){
					WCHAR *s;
					int slen=len+sizeof(WCHAR);
					list=tmp;
					s=calloc(slen,1);
					if(s){
						memcpy(s,start,len);
						list[list_index]=s;
					}
					list_count=list_count+1;
				}

			}
		}
		if(is_end)
			break;
		index++;
	}
	*out=list;
	*out_count=list_count;
	return TRUE;
}

int populate_entry_dlg(HWND hdlg,WCHAR *str)
{
	int result=FALSE;
	int i,len;
	WCHAR **list=0;
	int list_count=0;
	if(0==str)
		return result;
	len=wcslen(str);
	for(i=0;i<len;i++){
		WCHAR a=str[i];
		if(iswspace(a)){
			str[i]=L' ';
		}
	}
	split_words(str,&list,&list_count);
	{
		int desc_count=4096;
		WCHAR *desc=calloc(desc_count+1,sizeof(WCHAR));
		for(i=0;i<list_count;i++){
			switch(i){
			case 0:
				result=TRUE;
				SetDlgItemText(hdlg,IDC_EDIT_USER,list[i]);
				SetFocus(GetDlgItem(hdlg,IDC_EDIT_USER));
				break;
			case 1:
				SetDlgItemText(hdlg,IDC_EDIT_PWD,list[i]);
				SetFocus(GetDlgItem(hdlg,IDC_EDIT_PWD));
				break;
			default:
				_snwprintf(desc,desc_count,L"%s%s",desc,list[i]);
				desc[desc_count-1]=0;
				SetFocus(GetDlgItem(hdlg,IDC_EDIT_DESC));
				break;
			}
		}
		SetDlgItemText(hdlg,IDC_EDIT_DESC,desc);
		free(desc);
	}
	free_split_words(&list,&list_count);
	return result;
}

void add_user_cache(WCHAR *user)
{
	const WCHAR *section=L"USER_CACHE";
	const WCHAR *count_key=L"COUNT";
	int i,count=0;
	int update_count=FALSE;
	int target=-1;
	get_ini_val(section,count_key,&count);
	for(i=0;i<count;i++){
		WCHAR *entry=0;
		WCHAR key[80]={0};
		int found=FALSE;
		_snwprintf(key,_countof(key),L"ENTRY%i",i);
		get_ini_str(section,key,&entry);
		if(entry){
			if(0==entry[0] && target<0){
				target=i;
			}else if(0==wcscmp(entry,user)){
				target=i;
				found=TRUE;
			}
			free(entry);
		}else if(target<0){
			target=i;
		}
		if(found){
			break;
		}
	}
	if(target<0){
		target=count;
		count++;
		update_count=TRUE;
	}
	if(target>0){
		WCHAR *tmp=0;
		WCHAR key[80]={0};
		const WCHAR *entry0=L"ENTRY0";
		_snwprintf(key,_countof(key),L"ENTRY%i",target);
		get_ini_str(section,entry0,&tmp);
		if(tmp){
			set_ini_str(section,key,tmp);
		}else{
			set_ini_str(section,key,L"");
		}
		set_ini_str(section,entry0,user);
		free(tmp);
	}else{
		set_ini_str(section,L"ENTRY0",user);
	}
	if(update_count)
		set_ini_val(section,count_key,count);

}

void delete_user_cache(WCHAR *user)
{
	const WCHAR *section=L"USER_CACHE";
	const WCHAR *count_key=L"COUNT";
	int i,count=0;
	get_ini_val(section,count_key,&count);
	for(i=0;i<count;i++){
		WCHAR *entry=0;
		WCHAR key[80]={0};
		_snwprintf(key,_countof(key),L"ENTRY%i",i);
		get_ini_str(section,key,&entry);
		if(entry){
			if(0==wcscmp(entry,user)){
				set_ini_str(section,key,L"");
				if(i==(count-1)){
					set_ini_val(section,count_key,count-1);
				}
			}
			free(entry);
		}
	}

}

BOOL CALLBACK user_select_dlg(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hedit=0;
	const WCHAR *section=L"USER_CACHE";
	switch(msg){
	case WM_INITDIALOG:
		hedit=(HWND)lparam;
		if(hedit){
			RECT rect={0};
			GetWindowRect(hedit,&rect);
			SetWindowPos(hwnd,NULL,rect.left,rect.bottom,0,0,SWP_NOZORDER|SWP_NOSIZE);
		}
		{
			int i,count=0;
			HWND huser=GetDlgItem(hwnd,IDC_USER_HOTLIST);
			get_ini_val(section,L"COUNT",&count);
			for(i=0;i<count;i++){
				WCHAR key[80]={0};
				WCHAR *user=0;
				_snwprintf(key,_countof(key),L"ENTRY%i",i);
				get_ini_str(section,key,&user);
				if(user){
					if(user[0]){
						SendMessage(huser,LB_ADDSTRING,0,(LPARAM)user);
					}
					free(user);
				}
			}
			SendMessage(huser,LB_SETCURSEL,0,0);
			huser=GetDlgItem(hwnd,IDC_USER_ALL);
			count=g_pwd_list.count;
			for(i=0;i<count;i++){
				PWD_ENTRY *entry=&g_pwd_list.list[i];
				if(entry->user && entry->user[0]){
					SendMessage(huser,LB_ADDSTRING,0,(LPARAM)entry->user);
				}
			}
		}
		break;
	case WM_COMMAND:
		{
			int id=LOWORD(wparam);
			{
				char tmp[80]={0};
				_snprintf(tmp,sizeof(tmp),"ID=%i code=%i\n",LOWORD(wparam),HIWORD(wparam));
				OutputDebugStringA(tmp);
			}
			switch(id){
			case IDC_ADD:
				{
					HWND hlist=GetDlgItem(hwnd,IDC_USER_ALL);
					HWND hhot=GetDlgItem(hwnd,IDC_USER_HOTLIST);
					int index=SendMessage(hlist,LB_GETCURSEL,0,0);
					if(index>=0){
						int len=SendMessage(hlist,LB_GETTEXTLEN,index,0);
						if(len>0){
							WCHAR *tmp=calloc((len+1)*sizeof(WCHAR),1);
							if(tmp){
								SendMessage(hlist,LB_GETTEXT,index,(LPARAM)tmp);
								index=SendMessage(hhot,LB_FINDSTRINGEXACT,-1,(LPARAM)tmp);
								if(index<0)
									index=SendMessage(hhot,LB_ADDSTRING,0,(LPARAM)tmp);
								if(index>=0)
									SendMessage(hhot,LB_SETCURSEL,index,0);
								add_user_cache(tmp);
								free(tmp);
							}
						}
					}
				}
				break;
			case IDC_DELETE:
				{
					HWND hhot=GetDlgItem(hwnd,IDC_USER_HOTLIST);
					if(hhot){
						int index=SendMessage(hhot,LB_GETCURSEL,0,0);
						if(index>=0){
							WCHAR tmp[80]={0};
							SendMessage(hhot,LB_GETTEXT,index,(LPARAM)tmp);
							delete_user_cache(tmp);
							SendMessage(hhot,LB_DELETESTRING,index,0);
						}
						SendMessage(hhot,LB_SETCURSEL,0,0);
					}
				}
				break;
			case IDC_USER_HOTLIST:
			case IDC_USER_ALL:
				{
					int code=HIWORD(wparam);
					int is_dbl_click=(LBN_DBLCLK==code);
					if(is_dbl_click){
						HWND hlist=(HWND)lparam;
						int index=SendMessage(hlist,LB_GETCURSEL,0,0);
						if(index>=0){
							int len=SendMessage(hlist,LB_GETTEXTLEN,index,0);
							if(len>0){
								WCHAR *tmp=calloc((len+1)*sizeof(WCHAR),1);
								if(tmp){
									SendMessage(hlist,LB_GETTEXT,index,(LPARAM)tmp);
									if(hedit){
										SetWindowText(hedit,tmp);
										add_user_cache(tmp);

									}
									free(tmp);
									EndDialog(hwnd,0);
								}
							}
						}
					}
				}
				break;
			case IDCANCEL:
				EndDialog(hwnd,0);
				break;
			case IDOK:
				if(hedit)
				{
					WCHAR tmp[80]={0};
					HWND huser=GetDlgItem(hwnd,IDC_USER_HOTLIST);
					int index=SendMessage(huser,LB_GETCURSEL,0,0);
					if(index>=0){
						SendMessage(huser,LB_GETTEXT,index,(LPARAM)tmp);
						if(tmp[0]){
							add_user_cache(tmp);
							SetWindowText(hedit,tmp);
						}
					}
				}
				EndDialog(hwnd,0);
				break;
			}
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK entry_dlg(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int code=0;
	static int lv_item_index=0;
	static int lparam_val=0;
	static HWND hlist=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			int set_focus_user=TRUE;
			hlist=GetDlgItem(ghdlg,IDC_LISTVIEW);
			lv_item_index=get_focused_item(hlist);
			if(0==hlist)
				EndDialog(hwnd,0);
			code=lparam;
			switch(code){
			case IDC_EDIT:
				{
					WCHAR *user=0,*pwd=0,*desc=0;
					int res;
					if(-1==lv_item_index)
						EndDialog(hwnd,0);
					lparam_val=-1;
					get_entry_lparam(hlist,lv_item_index,&lparam_val);
					res=get_entry_text(hlist,lv_item_index,&user,&pwd,&desc);
					if(res){
						SetDlgItemTextW(hwnd,IDC_EDIT_USER,user);
						SetDlgItemTextW(hwnd,IDC_EDIT_PWD,pwd);
						SetDlgItemTextW(hwnd,IDC_EDIT_DESC,desc);
					}
					free(user);
					free(pwd);
					free(desc);
				}
				break;
			case IDC_PASTE:
				{
					WCHAR *str;
					str=get_clipboard();
					if(0==str)
						EndDialog(hwnd,0);
					if(!populate_entry_dlg(hwnd,str))
						EndDialog(hwnd,0);
					set_focus_user=FALSE;
				}
				break;
			}
			{
				int list[3]={IDC_EDIT_USER,IDC_EDIT_PWD,IDC_EDIT_DESC};
				setup_edit_controls(hwnd,list,_countof(list));
			}
			if(set_focus_user){
				HWND huser=GetDlgItem(hwnd,IDC_EDIT_USER);
				if(huser)
					SetFocus(huser);
			}
		}
		break;
	case WM_COMMAND:
		{
			int id=LOWORD(wparam);
			switch(id){
			case IDOK:
				{
					HWND hfocus;
					char name[40]={0};
					hfocus=GetFocus();
					GetClassNameA(hfocus,name,sizeof(name));
					strlwr(name);
					if(strstr(name,"edit")){
						int ext=GetKeyState(VK_CONTROL)&0x8000;
						SendMessage(hwnd,WM_NEXTDLGCTL,ext,0);
						return FALSE;
					}
				}
				switch(code){
				case IDC_PASTE:
				case IDC_EDIT:
				case IDC_ADD:
					{
						WCHAR *user=0,*pwd=0,*desc=0;
						if(get_hwnd_wchar(GetDlgItem(hwnd,IDC_EDIT_USER),&user)
							&& get_hwnd_wchar(GetDlgItem(hwnd,IDC_EDIT_PWD),&pwd)
							&& get_hwnd_wchar(GetDlgItem(hwnd,IDC_EDIT_DESC),&desc))
						{
							if(IDC_EDIT==code){
								set_entry_text(hlist,lv_item_index,user,pwd,desc);
								update_pwd_entry(&g_pwd_list,lparam_val,user,pwd,desc);
							}else{
								if(is_valid_entry(user,pwd,desc)){
									int res;
									res=add_pwd_entry(&g_pwd_list,user,pwd,desc);
									if(res)
										add_entry_text(hlist,g_pwd_list.count-1,user,pwd,desc);
								}
							}
						}
						free(user);
						free(pwd);
						free(desc);
						auto_set_list_width(hlist);
					}
					break;
				}
				EndDialog(hwnd,id);
				break;
			case IDCANCEL:
				EndDialog(hwnd,id);
				break;
			case IDC_PWD_BTN:
				show_pwd_dlg(hwnd,GetDlgItem(hwnd,IDC_EDIT_PWD),ghinstance);
				break;
			case IDC_USER_BTN:
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_USER_DLG),hwnd,user_select_dlg,(LPARAM)GetDlgItem(hwnd,IDC_EDIT_USER));
				break;
			}
		}
		break;
	}
	return FALSE;
}
int filter_list(HWND hlist,WCHAR *str)
{
	WCHAR **word_list=0;
	int i,word_count=0;
	PWD_LIST tmp_list={0};
	if(0==wcslen(str)){
		populate_listview(hlist,&g_pwd_list);
		return TRUE;
	}
	split_words(str,&word_list,&word_count);
	for(i=0;i<g_pwd_list.count;i++){
		PWD_ENTRY *entry=&g_pwd_list.list[i];
		add_pwd_entry(&tmp_list,entry->user,entry->pwd,entry->desc);
	}
	{
		int j;
		for(j=0;j<tmp_list.count;j++){
			int i;
			int match_count=0;
			PWD_ENTRY *entry=&tmp_list.list[j];
			WCHAR *desc=wcsdup(entry->desc);
			wcslwr(desc);
			for(i=0;i<word_count;i++){
				const WCHAR *token=word_list[i];
				WCHAR *ptr;
				ptr=wcsstr(desc,token);
				if(ptr){
					match_count++;
				}
			}
			if(match_count!=word_count){
				free(entry->desc);
				free(entry->pwd);
				free(entry->user);
				entry->desc=0;
				entry->pwd=0;
				entry->user=0;
			}
			
		}
	}
	populate_listview(hlist,&tmp_list);
	free_pwd_list(&tmp_list);
	free_split_words(&word_list,&word_count);
	return TRUE;
}
int lv_get_column_count(HWND hlistview)
{
	HWND header;
	int count=0;
	header=(HWND)SendMessage(hlistview,LVM_GETHEADER,0,0);
	if(header!=0){
		count=SendMessage(header,HDM_GETITEMCOUNT,0,0);
		if(count<0)
			count=0;
	}
	return count;
}
int draw_item(DRAWITEMSTRUCT *di,int selected_col)
{
	int i,count,xpos;
	int textcolor,bgcolor;
	RECT bound_rect={0},client_rect={0};
	static HBRUSH hbrush=0;

	if(hbrush==0){
		LOGBRUSH brush={0};
		DWORD color=GetSysColor(COLOR_HIGHLIGHT);
		brush.lbColor=color;
		brush.lbStyle=BS_SOLID;
		hbrush=CreateBrushIndirect(&brush);
	}
	count=lv_get_column_count(di->hwndItem);

	GetClientRect(di->hwndItem,&client_rect);

	bgcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW);
	textcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT);
	xpos=0;
	for(i=0;i<count;i++){
		WCHAR text[512]={0};
		LV_ITEM lvi={0};
		RECT rect;
		int width,style;

		width=ListView_GetColumnWidth(di->hwndItem,i);

		rect=di->rcItem;
		rect.left+=xpos;
		rect.right=rect.left+width;
		xpos+=width;


		if(rect.right>=0 && rect.left<=client_rect.right){
			lvi.mask=LVIF_TEXT;
			lvi.iItem=di->itemID;
			lvi.iSubItem=i;
			lvi.pszText=text;
			lvi.cchTextMax=sizeof(text);

			ListView_GetItemText(di->hwndItem,di->itemID,i,text,sizeof(text));
			text[sizeof(text)-1]=0;

			if(selected_col==i){
				bound_rect=rect;
				bound_rect.bottom++;
				bound_rect.right++;
			}
			if( (di->itemState&(ODS_FOCUS|ODS_SELECTED))==(ODS_FOCUS|ODS_SELECTED) && selected_col==i){
				FillRect(di->hDC,&rect,hbrush);
			}
			else{
				FillRect(di->hDC,&rect,(HBRUSH)(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT+1:COLOR_WINDOW+1));
			}
			{
				RECT tmprect=rect;
				tmprect.bottom++;
				tmprect.right++;
				FrameRect(di->hDC,&tmprect,GetStockObject(DKGRAY_BRUSH));
			}

			if(text[0]!=0){
				SetTextColor(di->hDC,textcolor);
				SetBkColor(di->hDC,bgcolor);

				style=DT_LEFT|DT_NOPREFIX;

				rect.left++;
				rect.right--;
				DrawText(di->hDC,text,-1,&rect,style);
			}
		}
	}
	if(di->itemState&ODS_FOCUS){
		SetTextColor(di->hDC,0x0000FF);
		bound_rect.right--;
		bound_rect.bottom--;
		DrawFocusRect(di->hDC,&bound_rect);
	}
	return TRUE;

}
void restore_position(HWND hwnd)
{
	int x=0,y=0;
	HWND hdesk;
	RECT rect={0};
	RECT app_rect={0};
	int w,h;
	GetWindowRect(hwnd,&app_rect);
	w=app_rect.right-app_rect.left;
	h=app_rect.bottom-app_rect.top;
	hdesk=GetDesktopWindow();
	if(hdesk){
		int cx,cy;
		GetClientRect(hdesk,&rect);
		cx=(rect.right-rect.left)/2;
		cy=(rect.bottom-rect.top)/2;
		cx-=w/2;
		cy-=h/2;
		if(cx<0)
			cx=0;
		if(cy<0)
			cy=0;
		x=cx;
		y=cy;
	}
	get_ini_val(SECTION_WINDOW,KEY_XPOS,&x);
	get_ini_val(SECTION_WINDOW,KEY_YPOS,&y);
	if(hdesk){
		if((x+w)>rect.right)
			x=rect.right-w;
		if((y+h)>rect.bottom)
			y=rect.bottom-h;
		if(x<rect.left)
			x=rect.left;
		if(y<rect.top)
			y=rect.top;
	}
	SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);
}
void save_position(HWND hwnd)
{
	RECT rect={0};
	GetWindowRect(hwnd,&rect);
	set_ini_val(SECTION_WINDOW,KEY_XPOS,rect.left);
	set_ini_val(SECTION_WINDOW,KEY_YPOS,rect.top);
}
void quit_dialog(HWND hwnd)
{
	save_position(hwnd);
	PostQuitMessage(0);
}
int load_icon(HWND hwnd)
{
	static HICON hIcon=0;
	if(0==hIcon)
		hIcon=LoadIcon(ghinstance,MAKEINTRESOURCE(IDI_ICON));
    if(hIcon){
		SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
		SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		return TRUE;
	}
	return FALSE;
}
void clear_all_item_state(HWND hlist)
{
	int i,count;
	count=ListView_GetItemCount(hlist);
	for(i=0;i<count;i++){
		ListView_SetItemState(hlist,i,0,LVIS_SELECTED|LVIS_FOCUSED);
	}
}

BOOL CALLBACK dlg_func(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int selected_column=0;
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
			load_pwd_list(&g_pwd_list);
			load_pwd_list(&g_first_pwd_list);
			hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
			populate_listview(hlist,&g_pwd_list);
			restore_position(hwnd);
			load_icon(hwnd);
			{
				int list[1]={IDC_FILTER_DESC};
				setup_edit_controls(hwnd,list,1);
			}
			{
				ListView_SetItemState(hlist,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
				SetFocus(hlist);
			}
		}
		break;
	case WM_SIZE:
		AnchorResize(hwnd,anchor_main_dlg,_countof(anchor_main_dlg));
		break;
	case WM_SIZING:
		{
			RECT dsize={0,0,500,300};
			RECT *size=(RECT*)lparam;
			if(size){
				return ClampMinWindowSize(&dsize,wparam,size);
			}
		}
		break;
	case WM_NOTIFY:
		{
			int id=wparam;
			NMHDR *hdr=(NMHDR*)lparam;
			switch(id){
			case IDC_LISTVIEW:
				{
					int redraw=FALSE;
					int focused_index=-1;
					HWND hlist=hdr->hwndFrom;
					switch(hdr->code){
					case LVN_KEYDOWN:
						{
							NMLVKEYDOWN *key=(NMLVKEYDOWN*)lparam;
							switch(key->wVKey){
							case VK_INSERT:
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_ADD,0),0);
								break;
							case VK_RETURN:
							case VK_F2:
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_EDIT,0),0);
								break;
							case VK_DELETE:
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_DELETE,0),0);
								break;
							case VK_F5:
								populate_listview(hdr->hwndFrom,&g_pwd_list);
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_FILTER_DESC,EN_CHANGE),0);
								break;
							case 'V':
								if(GetKeyState(VK_CONTROL)&0x8000){
									PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_PASTE,0),0);
								}
								break;
							case 'C':
								if(GetKeyState(VK_CONTROL)&0x8000){
									int index=get_focused_item(hlist);
									if(index>=0){
										WCHAR tmp[512]={0};
										ListView_GetItemText(hlist,index,selected_column,tmp,_countof(tmp));
										copy_to_clip(tmp);
									}
								}
								break;
							case VK_LEFT:
								selected_column--;
								if(selected_column<0)
									selected_column=0;
								redraw=TRUE;
								break;
							case VK_RIGHT:
								selected_column++;
								if(selected_column>2)
									selected_column=2;
								redraw=TRUE;
								break;
							}
						}
						break;
					case NM_RCLICK:
					case NM_CLICK:
						{
							LV_HITTESTINFO lvhit={0};
							GetCursorPos(&lvhit.pt);
							ScreenToClient(hlist,&lvhit.pt);
							ListView_SubItemHitTest(hlist,&lvhit);
							{
								int index=lvhit.iSubItem;
								if(index>=0 && index<=2){
									selected_column=index;
									focused_index=lvhit.iItem;
									redraw=TRUE;
								}
							}
						}
						break;
					case NM_DBLCLK:
						{
							int index;
							index=get_focused_item(hlist);
							if(index>=0){
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_EDIT,0),0);
							}
						}
						break;
					}
					if(redraw){
						if(focused_index<0)
							focused_index=get_focused_item(hlist);
						if(focused_index<0){
							focused_index=0;
							ListView_SetItemState(hlist,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
						}
						if(focused_index>=0)
							ListView_RedrawItems(hlist,focused_index,focused_index);
						UpdateWindow(hlist);
					}
				}
				break;
			}
		}
		break;
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *di=(DRAWITEMSTRUCT *)lparam;
			if(di!=0 && di->CtlType==ODT_LISTVIEW){
				draw_item(di,selected_column);
				return TRUE;
			}
		}
		break;
	case WM_APP:
		{
			int code=lparam;
			HWND hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
			int index=get_focused_item(hlist);
			int copy_text=FALSE;
			if(index<0){
				ListView_SetItemState(hlist,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
				index=0;
			}
			switch(code){
			case 1:
				selected_column=0;
				copy_text=TRUE;
				break;
			case 2:
				selected_column=1;
				copy_text=TRUE;
				break;
			case 3:
				selected_column=2;
				copy_text=TRUE;
				break;
			}
			ListView_RedrawItems(hlist,index,index);
			UpdateWindow(hlist);
			if(copy_text){
				WCHAR tmp[512]={0};
				ListView_GetItemText(hlist,index,selected_column,tmp,_countof(tmp));
				copy_to_clip(tmp);
			}
		}
		break;
	case WM_HELP:
		{
			static int showing=FALSE;
			WCHAR *tmp;
			int tmp_len=0x10000;
			const WCHAR *text=  L"ctrl+1 = copy 1st col\2\n"
								L"ctrl+2 = copy 2nd col\r\n"
								L"ctrl+3 = copy 3rd col\r\n"
								L"ctrl+E = set focus on edit\r\n"
								L"ctrl+L = set focus on list\r\n"
								L"ctrl+S = save list\r\n"
								L"F2 = edit entry\r\n"
								L"F5 = refresh\r\n"
								L"shift+delete = delete without prompt\r\n"
								L"ctrl+move = navigate in edit control\r\n";
			if(showing)
				break;
			showing=TRUE;
			tmp=calloc(tmp_len,sizeof(WCHAR));
			if(tmp){
				const WCHAR *fname=get_ini_fname();
				int len=tmp_len/sizeof(WCHAR);
				_snwprintf(tmp,len,L"%s\r\n%s",text,fname);
				MessageBox(hwnd,tmp,L"HELP",MB_OK);
				free(tmp);
			}else{
				MessageBox(hwnd,text,L"HELP",MB_OK);
			}
			showing=FALSE;
		}
		break;
	case WM_COMMAND:
		{
			int id=LOWORD(wparam);
			int code=HIWORD(wparam);
			switch(id){
			case IDC_LISTVIEW:
				break;
			case IDC_PASTE:
			case IDC_EDIT:
			case IDC_ADD:
				{
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_ENTRY),hwnd,&entry_dlg,id);
				}
				break;
			case IDC_DELETE:
				{
					int index;
					HWND hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
					index=get_focused_item(hlist);
					if(index>=0){
						int lv_param=-1;
						get_entry_lparam(hlist,index,&lv_param);
						if(lv_param>=0){
							int r;
							r=GetKeyState(VK_SHIFT)&0x8000;
							if(!r){
								r=MessageBox(hwnd,L"Are you sure you want to delete the entry?",L"WARNING",MB_OKCANCEL|MB_SYSTEMMODAL);
								if(IDOK!=r)
									break;
							}
							r=del_pwd_entry(&g_pwd_list,lv_param);
							if(r){
								int count;
								populate_listview(hlist,&g_pwd_list);
								SetDlgItemText(hwnd,IDC_FILTER_DESC,L"");
								count=ListView_GetItemCount(hlist);
								if(count){
									int state=LVIS_FOCUSED|LVIS_SELECTED;
									if(index>=count)
										index--;
									clear_all_item_state(hlist);
									ListView_SetItemState(hlist,index,state,state);
								}
							}
						}
					}
				}
				break;
			case IDC_ON_TOP:
				SetWindowPos(hwnd,IsDlgButtonChecked(hwnd,LOWORD(wparam))?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
				break;
			case IDC_PWD_GEN:
				show_pwd_dlg(hwnd,hwnd,ghinstance);
				break;
			case IDOK:
				break;
			case IDCANCEL:
				{
					int val;
					val=compare_pwdlist(&g_first_pwd_list,&g_pwd_list);
					if(val){
						int res=MessageBoxA(hwnd,"List has been modified.\r\nDo you want to exit?",
							"Warning",MB_OKCANCEL|MB_SYSTEMMODAL);
						if(IDOK!=res){
							return FALSE;
						}
					}
					quit_dialog(hwnd);
				}
				break;
			case IDC_SAVE:
				{
					int val;
					val=compare_pwdlist(&g_first_pwd_list,&g_pwd_list);
					if(0==val){
						MessageBoxA(hwnd,"Nothing has changed!","NO CHANGE",MB_OK|MB_SYSTEMMODAL);
						break;
					}
					save_pwd_list(&g_pwd_list);
					load_pwd_list(&g_pwd_list);
					load_pwd_list(&g_first_pwd_list);
				}
				break;
			case IDC_FILTER_DESC:
				{
					if(EN_CHANGE==code){
						WCHAR tmp[80]={0};
						int index,count;
						int state=LVIS_FOCUSED|LVIS_SELECTED;
						HWND hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
						index=get_focused_item(hlist);
						GetDlgItemText(hwnd,id,tmp,_countof(tmp));
						wcslwr(tmp);
						filter_list(hlist,tmp);
						count=ListView_GetItemCount(hlist);
						if(index<0 || index>=count){
							ListView_SetItemState(hlist,0,state,state);
						}else{
							ListView_SetItemState(hlist,index,state,state);
						}
					}
				}
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
	ghdlg=hwnd;
	ShowWindow(hwnd,SW_SHOW);
	while(1){
		MSG msg;
		int res;
		res=GetMessage(&msg,NULL,0,0);
		if(0==res || -1==res){
			break;
		}
		if(WM_KEYDOWN==msg.message){
			int wparam=msg.wParam;
			int ctrl=GetKeyState(VK_CONTROL)&0x8000;
			if(ctrl){
				if('1'==wparam || '2'==wparam || '3'==wparam){
					msg.hwnd=ghdlg;
					msg.message=WM_APP;
					msg.lParam=wparam-'1'+1;
				}else if('E'==wparam || 'L'==wparam || 'S'==wparam){
					HWND htarget=0;
					switch(wparam){
					case 'E':
						htarget=GetDlgItem(ghdlg,IDC_FILTER_DESC);
						break;
					case 'L':
						htarget=GetDlgItem(ghdlg,IDC_LISTVIEW);
						break;
					case 'S':
						PostMessage(ghdlg,WM_COMMAND,MAKEWPARAM(IDC_SAVE,0),0);
						break;
					}
					if(htarget){
						SetFocus(htarget);
						continue;
					}
				}else{
					HWND hedit=GetDlgItem(ghdlg,IDC_FILTER_DESC);
					if(hedit==msg.hwnd){
						switch(wparam){
						case 'C':
						case VK_UP:
						case VK_DOWN:
						case VK_LEFT:
						case VK_RIGHT:
						case VK_PRIOR:
						case VK_NEXT:
						case VK_HOME:
						case VK_END:
							{
								HWND hlist=GetDlgItem(ghdlg,IDC_LISTVIEW);
								msg.hwnd=hlist;
								PostMessage(ghdlg,WM_APP,-1,-1);
							}
							break;
						}
					}
				}
			}else{
				int lparam=msg.lParam;
				if(0==(lparam&(1<<24))){
					int key=MapVirtualKey(wparam,2);
					if( (key>=' ' && key<=0x7E)
						|| (VK_SPACE==wparam)
						|| (VK_BACK==wparam)){
						HWND hlist;
						hlist=GetDlgItem(ghdlg,IDC_LISTVIEW);
						if(msg.hwnd==hlist){
							msg.hwnd=GetDlgItem(ghdlg,IDC_FILTER_DESC);
						}
					}else{
						if(VK_RETURN==wparam){
							msg.wParam=VK_F2;
						}
					}
				}
			}
		}else if(WM_SYSKEYDOWN==msg.message){
			int lparam=msg.lParam;
			if(lparam&(1<<29)){
				if(VK_HOME==msg.wParam){
					HWND htmp=GetFocus();
					SendDlgItemMessage(ghdlg,IDC_ON_TOP,BM_CLICK,IDC_ON_TOP,0);
					if(htmp)
						SetFocus(htmp);
					continue;
				}
			}
		}
		if(!IsDialogMessage(hwnd,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
