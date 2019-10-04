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
	{IDC_SAVE_EXIT,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
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
	hlistview=CreateWindow(WC_LISTVIEW,L"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
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
int update_pwd_entry(&g_pwd_list,lparam_val,user,pwd,desc)
{
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
		x=get_string_width_wc(header,tmp,1);
		if(x>=val[0])
			val[0]=x;
		//---
		tmp=entry->pwd;
		val=&widths[1];
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
		col.mask=LVCF_WIDTH;
		ListView_GetColumn(hlist,i,&col);
		if(col.cx>widths[i])
			widths[i]=col.cx;
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
BOOL CALLBACK entry_dlg(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int code=0;
	static int item_index=0;
	static int lparam_val=0;
	static HWND hlist=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			hlist=GetDlgItem(ghdlg,IDC_LISTVIEW);
			item_index=get_focused_item(hlist);
			if(0==hlist)
				EndDialog(hwnd,0);
			code=lparam;
			switch(code){
			case IDC_EDIT:
				{
					WCHAR *user=0,*pwd=0,*desc=0;
					int res;
					if(-1==item_index)
						EndDialog(hwnd,0);
					lparam_val=-1;
					get_entry_lparam(hlist,item_index,&lparam_val);
					res=get_entry_text(hlist,item_index,&user,&pwd,&desc);
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
			}
			{
				int list[3]={IDC_EDIT_USER,IDC_EDIT_PWD,IDC_EDIT_DESC};
				HWND huser=GetDlgItem(hwnd,IDC_EDIT_USER);
				if(huser)
					SetFocus(huser);
				setup_edit_controls(hwnd,list,_countof(list));

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
				case IDC_EDIT:
				case IDC_ADD:
					{
						WCHAR *user=0,*pwd=0,*desc=0;
						if(get_hwnd_wchar(GetDlgItem(hwnd,IDC_EDIT_USER),&user)
							&& get_hwnd_wchar(GetDlgItem(hwnd,IDC_EDIT_PWD),&pwd)
							&& get_hwnd_wchar(GetDlgItem(hwnd,IDC_EDIT_DESC),&desc))
						{
							if(IDC_ADD==code){
								int res;
								res=add_pwd_entry(&g_pwd_list,user,pwd,desc);
								if(res)
									add_entry_text(hlist,g_pwd_list.count-1,user,pwd,desc);
							}else{
								set_entry_text(hlist,item_index,user,pwd,desc);
								update_pwd_entry(&g_pwd_list,lparam_val,user,pwd,desc);
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
			}
		}
		break;
	}
	return FALSE;
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
		WCHAR a=str[index]++;
		if(0==a || ' '==a){
			if(0==state){
				parse=TRUE;
				state=1;
			}
		}else{
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
int filter_list(HWND hlist,int col,WCHAR *str)
{
	WCHAR **word_list=0;
	int i,word_count=0;
	split_words(str,&word_list,&word_count);
	for(i=0;i<word_count;i++){
		
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
			load_pwd_list(&g_pwd_list);
			load_pwd_list(&g_first_pwd_list);
			hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
			populate_listview(hlist,&g_pwd_list);
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
					switch(hdr->code){
					case LVN_KEYDOWN:
						{
							NMLVKEYDOWN *key=(NMLVKEYDOWN*)lparam;
							switch(key->wVKey){
							case VK_INSERT:
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_ADD,0),0);
								break;
							case VK_F2:
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_EDIT,0),0);
								break;
							case VK_DELETE:
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_DELETE,0),0);
								break;
							case VK_F5:
								populate_listview(hdr->hwndFrom,&g_pwd_list);
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_FILTER_DESC,0),0);
								break;
							}
						}
						break;
					case NM_DBLCLK:
						{
							int index;
							HWND hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
							index=get_focused_item(hlist);
							if(index>=0){
								PostMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_EDIT,0),0);
							}
						}
						break;
					}

				}
				break;
			}
		}
		break;
	case WM_COMMAND:
		{
			int id=LOWORD(wparam);
			int code=HIWORD(wparam);
			switch(id){
			case IDC_LISTVIEW:
				break;
			case IDC_EDIT:
			case IDC_ADD:
				{
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_ENTRY),hwnd,&entry_dlg,id);
				}
				break;
			case IDC_DELETE:
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
					PostQuitMessage(0);
				}
				break;
			case IDC_SAVE_EXIT:
				save_pwd_list(&g_pwd_list);
				PostQuitMessage(0);
				break;
			case IDC_FILTER_DESC:
				{
					if(EN_CHANGE==code){
						WCHAR tmp[80]={0};
						HWND hlist=GetDlgItem(hwnd,IDC_LISTVIEW);
						GetDlgItemText(hwnd,id,tmp,_countof(tmp));
						filter_list(hlist,2,tmp);
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

		if(!IsDialogMessage(hwnd,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
