
#pragma once

#define RC_PRODUCT		"crvskkserv"
#define RC_VERSION		"2.0.2"
#define RC_VERSION_D	2,0,2,0
#define RC_TITLE		"crvskkserv (ver. 2.0.2)"
#define RC_AUTHOR		"nathancorvussolis"

#define APP_TITLE		L"crvskkserv"
#define APP_VERSION		L"2.0.2"
#define RES_VER RC_PRODUCT "/" RC_VERSION " "

#define REQ_END		'0'
#define REQ_KEY		'1'
#define REQ_VER		'2'
#define REQ_HST		'3'
#define REQ_CMP		'4'

#define REP_ERROR		'0'
#define REP_FOUND		'1'
#define REP_NOT_FOUND	'4'

#define INIVAL_SKKSERV		L"skkserv"
#define INIVAL_GOOGLECGIAPI	L"googlecgiapi"
#define INIVAL_SVR_SEP		L'/'

#define DICBUFSIZE	0x1000
#define RBUFSIZE	0x800

#define WM_TASKBARICON_0	(WM_USER + 1)

typedef struct {
	BOOL live;
	SOCKET sock;
} SERVINFO;

typedef std::pair<std::string, long> PAIR;
typedef std::map<std::string, long> MAP;

typedef std::vector<long> POS;
typedef struct {
	std::wstring path;
	POS pos;
	SOCKET sock;
} DICINFO;
typedef std::vector<DICINFO> VDICINFO;

extern LPCSTR EntriesAri;
extern LPCSTR EntriesNasi;
extern LPCWSTR RB;
extern LPCWSTR WB;

extern LPCWSTR title;
extern LPCSTR resver;
extern WCHAR serv_port[];
extern BOOL serv_loopback;
extern WCHAR googlecgiapi_url_prefix[];
extern WCHAR googlecgiapi_url_suffix[];
extern VDICINFO vdicinfo;

extern LPCWSTR inival_def_timeout;
extern LPCWSTR inival_googlecgiapi_encoding_euc;
extern LPCWSTR inival_googlecgiapi_encoding_utf8;

// server
int make_serv_sock(SERVINFO *servinfo, int servinfonum);
void disconnect(SOCKET &sock);
void comm(SOCKET &sock);
void comm_thread(void *p);
void listen_thread(void *p);

// search_dictionary
void init_search_dictionary(DICINFO &dicinfo);
void search_dictionary(DICINFO &dicinfo, const std::string &key, std::string &s);

// search_skkserv
void search_skkserv(DICINFO &dicinfo, const std::string &key, std::string &s);
void connect_skkserv(DICINFO &dicinfo);
BOOL get_skkserv_version(SOCKET &sock);

// search_google_cgiapi
void search_google_cgiapi(DICINFO &dicinfo, const std::string &key, std::string &s);
