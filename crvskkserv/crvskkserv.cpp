
#include "crvskkserv.h"
#include "resource.h"

#define MAX_DICNUM 64
LPCWSTR title = APP_TITLE;
LPCSTR resver = RES_VER;
HINSTANCE hInst;
NOTIFYICONDATAW nid;
BOOL bDlgShowed = FALSE;
WCHAR ini[MAX_PATH];
WCHAR serv_port[6];
BOOL serv_loopback = TRUE;
WCHAR googlecgiapi_url_prefix[INTERNET_MAX_URL_LENGTH];
WCHAR googlecgiapi_url_suffix[INTERNET_MAX_URL_LENGTH];
SERVINFO servinfo[FD_SETSIZE];
HANDLE hThread[FD_SETSIZE];
int servinfonum;
VDICINFO vdicinfo;
LPCWSTR inikey_port = L"port";
LPCWSTR inikey_loopback = L"loopback";
LPCWSTR inikey_googlecgiapi_url_prefix = L"googlecgiapi_url_prefix";
LPCWSTR inikey_googlecgiapi_url_suffix = L"googlecgiapi_url_suffix";
LPCWSTR inikey_dic = L"dic-";
CONST WCHAR inival_svr_sep = L'/';
LPCWSTR inival_def_googlecgiapi_url_prefix = L"https://www.google.com/transliterate?langpair=ja-Hira|ja&text=";
LPCWSTR inival_def_googlecgiapi_url_suffix = L",";
LPCWSTR inival_def_port = L"1178";
LPCWSTR inival_def_timeout = L"1000";
LPCWSTR inival_def_googlecgiapi_filter = L"[^A-Za-z0-9]+[a-z]";
LPCWSTR inival_def_googlecgiapi_annotation = L"G";
LPCWSTR inival_googlecgiapi_encoding_euc = L"euc";
LPCWSTR inival_googlecgiapi_encoding_utf8 = L"utf-8";
LPCSTR EntriesAri = ";; okuri-ari entries.\n";
LPCSTR EntriesNasi = ";; okuri-nasi entries.\n";
LPCWSTR modeRB = L"rb";
LPCWSTR modeWB = L"wb";

void GetIniFileName(LPWSTR ini, size_t len);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	WNDCLASSEXW wcex = {};
	HWND hWnd;
	WSADATA wsaData;
	int wsa = -1;
	INITCOMMONCONTROLSEX icex = {};

	_wsetlocale(LC_ALL, L"ja-JP");

	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	wsa = WSAStartup(WINSOCK_VERSION, &wsaData);
	switch (wsa)
	{
	case 0:
		//success
		break;
	case WSASYSNOTREADY:
	case WSAVERNOTSUPPORTED:
	case WSAEINPROGRESS:
	case WSAEPROCLIM:
	case WSAEFAULT:
	default:
		//error
		break;
	}

	GetIniFileName(ini, _countof(ini));

	hInst = hInstance;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_CRVSKKSERV));
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = title;
	wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCE(IDI_CRVSKKSERV));

	RegisterClassExW(&wcex);

	hWnd = CreateWindowW(title, title, WS_OVERLAPPEDWINDOW, 0, 0, 320, 200, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return 0;
	}

	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	WSACleanup();

	return (int)msg.wParam;
}

void GetIniFileName(LPWSTR inifile, size_t len)
{
	WCHAR drive[_MAX_DRIVE] = {};
	WCHAR dir[_MAX_DIR] = {};
	WCHAR fname[_MAX_FNAME] = {};
	WCHAR ext[_MAX_EXT] = {};

	GetModuleFileNameW(nullptr, inifile, (DWORD)len);
	_wsplitpath_s(inifile, drive, dir, fname, ext);
	_wmakepath_s(inifile, len, drive, dir, fname, L"ini");
}

void AddTaskbarIcon(HWND hWnd)
{
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCE(IDI_CRVSKKSERV));
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage = WM_TASKBARICON_0;
	wcscpy_s(nid.szTip, title);
	if (!Shell_NotifyIconW(NIM_ADD, &nid))
	{
		Sleep(100);
	}
}

void init_server()
{
	int i;
	DICINFO dicinfo;
	WCHAR key[8];
	WCHAR dicpath[MAX_PATH];

	GetPrivateProfileStringW(title, inikey_googlecgiapi_url_prefix, inival_def_googlecgiapi_url_prefix,
		googlecgiapi_url_prefix, _countof(googlecgiapi_url_prefix), ini);
	GetPrivateProfileStringW(title, inikey_googlecgiapi_url_suffix, inival_def_googlecgiapi_url_suffix,
		googlecgiapi_url_suffix, _countof(googlecgiapi_url_suffix), ini);

	GetPrivateProfileStringW(title, inikey_port, L"", serv_port, _countof(serv_port), ini);
	serv_loopback = GetPrivateProfileIntW(title, inikey_loopback, 1, ini);

	for (i = 1; i <= MAX_DICNUM; i++)
	{
		_snwprintf_s(key, _TRUNCATE, L"%s%d", inikey_dic, i);
		GetPrivateProfileStringW(title, key, L"", dicpath, _countof(dicpath), ini);
		if (dicpath[0] != L'\0')
		{
			dicinfo.path = dicpath;
			dicinfo.pos.clear();
			dicinfo.pos.shrink_to_fit();
			dicinfo.sock = INVALID_SOCKET;
			if (wcsncmp(dicpath, INIVAL_SKKSERV, wcslen(INIVAL_SKKSERV)) == 0)
			{
				connect_skkserv(dicinfo);
			}
			else if (wcsncmp(dicpath, INIVAL_GOOGLECGIAPI, wcslen(INIVAL_GOOGLECGIAPI)) == 0)
			{
			}
			else
			{
				init_search_dictionary(dicinfo);
			}
			vdicinfo.push_back(dicinfo);
		}
	}

	if (serv_port[0] == L'\0')
	{
		return;
	}

	for (i = 0; i < FD_SETSIZE; i++)
	{
		servinfo[i].live = FALSE;
		servinfo[i].sock = INVALID_SOCKET;
	}

	servinfonum = make_serv_sock(servinfo, _countof(servinfo));

	for (i = 0; i < servinfonum; i++)
	{
		_beginthread(listen_thread, 0, &servinfo[i]);
	}
}

void term_server()
{
	int i;
	VDICINFO::iterator vdicinfo_itr;

	for (i = 0; i < servinfonum; i++)
	{
		disconnect(servinfo[i].sock);
	}
	for (i = 0; i < servinfonum; i++)
	{
		while (servinfo[i].live)
		{
			Sleep(10);
		}
	}

	for (vdicinfo_itr = vdicinfo.begin(); vdicinfo_itr != vdicinfo.end(); vdicinfo_itr++)
	{
		disconnect(vdicinfo_itr->sock);
	}
	vdicinfo.clear();
}

INT_PTR CALLBACK DlgProcSKKServ(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hPDlg;
	HWND hWndListView;
	LVITEMW item = {};
	int index, count;
	WCHAR path[MAX_PATH];
	WCHAR host[256];
	WCHAR port[6];
	WCHAR timeout[8];

	switch (message)
	{
	case WM_INITDIALOG:
		hPDlg = (HWND)lParam;
		SetForegroundWindow(hDlg);

		SetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_PORT, inival_def_port);
		SetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_TIMEOUT, inival_def_timeout);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_HOST, host, _countof(host));
			GetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_PORT, port, _countof(port));
			GetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_TIMEOUT, timeout, _countof(timeout));
			_snwprintf_s(path, _TRUNCATE, L"%s%c%s%c%s%c%s%c", INIVAL_SKKSERV,
				INIVAL_SVR_SEP, host, INIVAL_SVR_SEP, port, INIVAL_SVR_SEP, timeout, INIVAL_SVR_SEP);

			hWndListView = GetDlgItem(hPDlg, IDC_LIST_SKK_DIC);
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			count = ListView_GetItemCount(hWndListView);
			if (index == -1)
			{
				index = count;
			}
			else
			{
				++index;
			}
			item.mask = LVIF_TEXT;
			item.pszText = path;
			item.iItem = index;
			item.iSubItem = 0;
			ListView_InsertItem(hWndListView, &item);
			ListView_SetItemState(hWndListView, index, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
			ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
			ListView_EnsureVisible(hWndListView, index, FALSE);

			if (ListView_GetItemCount(hWndListView) >= MAX_DICNUM)
			{
				EnableWindow(GetDlgItem(hPDlg, IDC_BUTTON_SKK_DIC_ADD), FALSE);
				EnableWindow(GetDlgItem(hPDlg, IDC_BUTTON_SKKSERV_ADD), FALSE);
				EnableWindow(GetDlgItem(hPDlg, IDC_BUTTON_GOOGLECGIAPI_ADD), FALSE);
			}

			EndDialog(hDlg, 0);
			break;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;

		default:
			break;
		}
		return (INT_PTR)TRUE;

	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hDlg, 0);
		return (INT_PTR)TRUE;

	default:
		break;
	}

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DlgProcGoogleCGIAPI(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hPDlg;
	HWND hWndListView;
	LVITEMW item = {};
	int	index, count;
	WCHAR path[MAX_PATH];
	WCHAR encoding[8];
	WCHAR filter[256];
	WCHAR comment[256];
	WCHAR timeout[8];

	switch (message)
	{
	case WM_INITDIALOG:
		hPDlg = (HWND)lParam;
		SetForegroundWindow(hDlg);

		SetDlgItemTextW(hDlg, IDC_EDIT_GOOGLECGIAPI_FILTER, inival_def_googlecgiapi_filter);
		SetDlgItemTextW(hDlg, IDC_EDIT_GOOGLECGIAPI_ANNOTATION, inival_def_googlecgiapi_annotation);
		SetDlgItemTextW(hDlg, IDC_EDIT_GOOGLECGIAPI_TIMEOUT, inival_def_timeout);
		CheckDlgButton(hDlg, IDC_RADIO_GOOGLECGIAPI_EUC, BST_CHECKED);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			wcscpy_s(encoding, inival_googlecgiapi_encoding_euc);
			if (IsDlgButtonChecked(hDlg, IDC_RADIO_GOOGLECGIAPI_UTF8) == BST_CHECKED)
			{
				wcscpy_s(encoding, inival_googlecgiapi_encoding_utf8);
			}
			GetDlgItemTextW(hDlg, IDC_EDIT_GOOGLECGIAPI_FILTER, filter, _countof(filter));
			GetDlgItemTextW(hDlg, IDC_EDIT_GOOGLECGIAPI_ANNOTATION, comment, _countof(comment));
			for (index = 0; index < _countof(comment) && comment[index] != L'\0'; index++)
			{
				if (comment[index] == L'/' || comment[index] == L';')
				{
					comment[index] = L'\x20';
				}
			}
			GetDlgItemTextW(hDlg, IDC_EDIT_GOOGLECGIAPI_TIMEOUT, timeout, _countof(timeout));
			_snwprintf_s(path, _TRUNCATE, L"%s%c%s%c%s%c%s%c%s%c", INIVAL_GOOGLECGIAPI, INIVAL_SVR_SEP,
				filter, INIVAL_SVR_SEP, comment, INIVAL_SVR_SEP, timeout, INIVAL_SVR_SEP, encoding, INIVAL_SVR_SEP);

			hWndListView = GetDlgItem(hPDlg, IDC_LIST_SKK_DIC);
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			count = ListView_GetItemCount(hWndListView);
			if (index == -1)
			{
				index = count;
			}
			else
			{
				++index;
			}
			item.mask = LVIF_TEXT;
			item.pszText = path;
			item.iItem = index;
			item.iSubItem = 0;
			ListView_InsertItem(hWndListView, &item);
			ListView_SetItemState(hWndListView, index, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
			ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
			ListView_EnsureVisible(hWndListView, index, FALSE);

			if (ListView_GetItemCount(hWndListView) >= MAX_DICNUM)
			{
				EnableWindow(GetDlgItem(hPDlg, IDC_BUTTON_SKK_DIC_ADD), FALSE);
				EnableWindow(GetDlgItem(hPDlg, IDC_BUTTON_SKKSERV_ADD), FALSE);
				EnableWindow(GetDlgItem(hPDlg, IDC_BUTTON_GOOGLECGIAPI_ADD), FALSE);
			}

			EndDialog(hDlg, 0);
			break;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;

		default:
			break;
		}
		return (INT_PTR)TRUE;

	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hDlg, 0);
		return (INT_PTR)TRUE;

	default:
		break;
	}

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DlgProcConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndListView;
	LV_COLUMNW lvc = {};
	LVITEMW item = {};
	OPENFILENAMEW ofn;
	WCHAR path[MAX_PATH] = {};
	WCHAR pathBak[MAX_PATH] = {};
	int index, count;
	VDICINFO::iterator vdicinfo_itr;
	WCHAR key[8];
	FILE *fp;

	switch (message)
	{
	case WM_INITDIALOG:
		SetForegroundWindow(hDlg);

		if (serv_port[0] == L'\0')
		{
			wcscpy_s(serv_port, inival_def_port);
		}
		SetDlgItemTextW(hDlg, IDC_EDIT_SERV_PORT, serv_port);

		CheckDlgButton(hDlg, IDC_CHECKBOX_SERV_LOOPBACK, (serv_loopback ? BST_CHECKED : BST_UNCHECKED));

		hWndListView = GetDlgItem(hDlg, IDC_LIST_SKK_DIC);
		ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_CENTER;
		lvc.iSubItem = 0;
		lvc.cx = 220;
		lvc.pszText = L"";
		ListView_InsertColumn(hWndListView, 0, &lvc);

		index = 0;
		for (vdicinfo_itr = vdicinfo.begin(); vdicinfo_itr != vdicinfo.end(); vdicinfo_itr++)
		{
			item.mask = LVIF_TEXT;
			item.pszText = (LPWSTR)vdicinfo_itr->path.c_str();
			item.iItem = index;
			item.iSubItem = 0;
			ListView_InsertItem(hWndListView, &item);
			index++;
		}

		ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);

		if (ListView_GetItemCount(hWndListView) >= MAX_DICNUM)
		{
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SKK_DIC_ADD), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SKKSERV_ADD), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_GOOGLECGIAPI_ADD), FALSE);
		}
		return (INT_PTR)TRUE;

	case WM_DPICHANGED:
		hWndListView = GetDlgItem(hDlg, IDC_LIST_SKK_DIC);
		ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		hWndListView = GetDlgItem(hDlg, IDC_LIST_SKK_DIC);

		switch (LOWORD(wParam))
		{
		case IDOK:
			_wfopen_s(&fp, ini, modeWB);
			if (fp != nullptr)
			{
				fwrite("\xFF\xFE", 2, 1, fp);
				fclose(fp);
			}

			GetDlgItemTextW(hDlg, IDC_EDIT_SERV_PORT, serv_port, _countof(serv_port));
			WritePrivateProfileStringW(title, inikey_port, serv_port, ini);

			(IsDlgButtonChecked(hDlg, IDC_CHECKBOX_SERV_LOOPBACK) == BST_CHECKED) ?
				(serv_loopback = TRUE) : (serv_loopback = FALSE);
			_snwprintf_s(key, _TRUNCATE, L"%d", serv_loopback);
			WritePrivateProfileStringW(title, inikey_loopback, key, ini);

			WritePrivateProfileStringW(title, inikey_googlecgiapi_url_prefix, googlecgiapi_url_prefix, ini);
			WritePrivateProfileStringW(title, inikey_googlecgiapi_url_suffix, googlecgiapi_url_suffix, ini);

			count = ListView_GetItemCount(hWndListView);
			for (index = 0; index < MAX_DICNUM && index < count; index++)
			{
				_snwprintf_s(key, _TRUNCATE, L"%s%d", inikey_dic, index + 1);
				ListView_GetItemText(hWndListView, index, 0, path, _countof(path));
				WritePrivateProfileStringW(title, key, path, ini);
			}

			EndDialog(hDlg, 0);
			term_server();
			init_server();
			break;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			break;

		case IDC_BUTTON_SKK_DIC_UP:
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			if (index > 0)
			{
				ListView_GetItemText(hWndListView, index - 1, 0, pathBak, _countof(pathBak));
				ListView_GetItemText(hWndListView, index, 0, path, _countof(path));
				ListView_SetItemText(hWndListView, index - 1, 0, path);
				ListView_SetItemText(hWndListView, index, 0, pathBak);
				ListView_SetItemState(hWndListView, index - 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_EnsureVisible(hWndListView, index - 1, FALSE);
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_DOWN:
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			count = ListView_GetItemCount(hWndListView);
			if (index >= 0 && index < count - 1)
			{
				ListView_GetItemText(hWndListView, index + 1, 0, pathBak, _countof(pathBak));
				ListView_GetItemText(hWndListView, index, 0, path, _countof(path));
				ListView_SetItemText(hWndListView, index + 1, 0, path);
				ListView_SetItemText(hWndListView, index, 0, pathBak);
				ListView_SetItemState(hWndListView, index + 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_EnsureVisible(hWndListView, index + 1, FALSE);
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_ADD:
			path[0] = L'\0';
			ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
			ofn.lStructSize = sizeof(OPENFILENAMEW);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = path;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"ファイル追加";
			ofn.Flags = OFN_FILEMUSTEXIST;
			if (GetOpenFileNameW(&ofn) != 0)
			{
				index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
				count = ListView_GetItemCount(hWndListView);
				if (index == -1)
				{
					index = count;
				}
				else
				{
					++index;
				}
				item.mask = LVIF_TEXT;
				item.pszText = path;
				item.iItem = index;
				item.iSubItem = 0;
				ListView_InsertItem(hWndListView, &item);
				ListView_SetItemState(hWndListView, index, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
				ListView_EnsureVisible(hWndListView, index, FALSE);

				if (ListView_GetItemCount(hWndListView) >= MAX_DICNUM)
				{
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SKK_DIC_ADD), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SKKSERV_ADD), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_GOOGLECGIAPI_ADD), FALSE);
				}
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKKSERV_ADD:
			DialogBoxParamW(hInst, MAKEINTRESOURCE(IDD_DIALOG_SKKSRV), hDlg, DlgProcSKKServ, (LPARAM)hDlg);
			return (INT_PTR)TRUE;

		case IDC_BUTTON_GOOGLECGIAPI_ADD:
			DialogBoxParamW(hInst, MAKEINTRESOURCE(IDD_DIALOG_GOOGLECGIAPI), hDlg, DlgProcGoogleCGIAPI, (LPARAM)hDlg);
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_DEL:
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			if (index != -1)
			{
				ListView_DeleteItem(hWndListView, index);
				ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);

				if (ListView_GetItemCount(hWndListView) < MAX_DICNUM)
				{
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SKK_DIC_ADD), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SKKSERV_ADD), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_GOOGLECGIAPI_ADD), TRUE);
				}
			}
			return (INT_PTR)TRUE;

		default:
			break;
		}
		break;

	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hDlg, 0);
		return (INT_PTR)TRUE;

	default:
		break;
	}

	return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static UINT s_uTaskbarRestart;
	HMENU hMenu;
	HMENU hSubMenu;
	POINT pt;

	switch (message)
	{
	case WM_CREATE:
		s_uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
		init_server();
		AddTaskbarIcon(hWnd);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_CONFIG:
			if (!bDlgShowed)
			{
				bDlgShowed = TRUE;
				DialogBoxW(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONFIG), hWnd, DlgProcConfig);
				bDlgShowed = FALSE;
			}
			else
			{
				SetForegroundWindow(hWnd);
			}
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			break;
		}
		break;

	case WM_TASKBARICON_0:
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
			if (!bDlgShowed)
			{
				bDlgShowed = TRUE;
				DialogBoxW(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONFIG), hWnd, DlgProcConfig);
				bDlgShowed = FALSE;
			}
			else
			{
				SetForegroundWindow(hWnd);
			}
			break;
		case WM_RBUTTONDOWN:
			hMenu = LoadMenuW(hInst, MAKEINTRESOURCE(IDR_MENU));
			if (hMenu)
			{
				GetCursorPos(&pt);
				SetForegroundWindow(hWnd);
				hSubMenu = GetSubMenu(hMenu, 0);
				if (hSubMenu)
				{
					TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
						pt.x, pt.y, 0, hWnd, nullptr);
				}
				DestroyMenu(hMenu);
			}
			break;
		default:
			break;
		}
		break;

	case WM_DESTROY:
	case WM_ENDSESSION:
		Shell_NotifyIconW(NIM_DELETE, &nid);
		term_server();
		break;

	default:
		if (message == s_uTaskbarRestart)
		{
			AddTaskbarIcon(hWnd);
		}
		break;
	}

	return DefWindowProcW(hWnd, message, wParam, lParam);
}
