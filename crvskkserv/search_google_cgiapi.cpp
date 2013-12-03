
#include "crvskkserv.h"
#include "eucjis2004.h"

//	http://www.google.com/intl/ja/ime/cgiapi.html

LPCWSTR useragent = L"Mozilla/5.0 (compatible; " APP_TITLE L"/" APP_VERSION L")";

std::string wstring_to_utf8_string(std::wstring &s)
{
	std::string ret;

	int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, NULL, 0, NULL, NULL);
	if(len > 0)
	{
		try
		{
			LPSTR utf8 = new CHAR[len];
			if(WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, utf8, len, NULL, NULL) > 0)
			{
				ret = utf8;
			}
			delete[] utf8;
		}
		catch(...)
		{
		}
	}

	return ret;
}

std::wstring utf8_string_to_wstring(std::string &s)
{
	std::wstring ret;

	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
	if(len > 0)
	{
		try
		{
			LPWSTR wcs = new WCHAR[len];
			if(MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, wcs, len) > 0)
			{
				ret = wcs;
			}
			delete[] wcs;
		}
		catch(...)
		{
		}
	}

	return ret;
}

std::string wstring_to_euc_jis_2004_string(std::wstring &s)
{
	std::string ret;
	size_t dstsize = -1;

	if(WideCharToEucJis2004(s.c_str(), NULL, NULL, &dstsize))
	{
		try
		{
			LPSTR cs = new CHAR[dstsize];
			if(WideCharToEucJis2004(s.c_str(), NULL, cs, &dstsize))
			{
				ret = cs;
			}
			delete[] cs;
		}
		catch(...)
		{
		}
	}

	return ret;
}

std::wstring euc_jis_2004_string_to_wstring(std::string &s)
{
	std::wstring ret;
	size_t dstsize = -1;

	if(EucJis2004ToWideChar(s.c_str(), NULL, NULL, &dstsize))
	{
		try
		{
			LPWSTR wcs = new WCHAR[dstsize];
			if(EucJis2004ToWideChar(s.c_str(), NULL, wcs, &dstsize))
			{
				ret = wcs;
			}
			delete[] wcs;
		}
		catch(...)
		{
		}
	}

	return ret;
}

void split_google_cgiapi_path(DICINFO &dicinfo, std::wstring &filter, std::wstring &comment, std::wstring &timeout, std::wstring &encoding)
{
	size_t is, ie;

	is = 0;
	ie = dicinfo.path.find_first_of(INIVAL_GOOGLECGIAPI, is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	filter = dicinfo.path.substr(is, ie - is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	comment = dicinfo.path.substr(is, ie - is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	timeout = dicinfo.path.substr(is, ie - is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	encoding = dicinfo.path.substr(is, ie - is);
}

void search_google_cgiapi(DICINFO &dicinfo, LPCSTR key, std::string &s)
{
	HINTERNET hInet;
	HINTERNET hUrl;
	WCHAR url[INTERNET_MAX_URL_LENGTH];
	CHAR buf[BUFSIZE];
	DWORD bytesRead = 0;
	WCHAR pe[4];
	size_t i;
	std::wstring wjson, wjson_tmp, c;
	std::wregex wreg;
	std::wsmatch wres;
	std::wstring wfmt;
	std::wstring filter, annotation, timeout, encoding;
	DWORD dwTimeout;
	std::wstring wkey;
	std::string ckey;
	size_t srcsize, dstsize;

	//接続情報取得
	split_google_cgiapi_path(dicinfo, filter, annotation, timeout, encoding);

	//文字コードチェック、UTF-8変換
	if(encoding == inival_googlecgiapi_encoding_euc)
	{
		srcsize = strlen(key) - 2;
		dstsize = -1;
		if(EucJis2004ToWideChar(key + 1, &srcsize, NULL, &dstsize) == FALSE)
		{
			return;
		}
		wkey = euc_jis_2004_string_to_wstring(std::string(key).substr(1, strlen(key) - 2));
		ckey = wstring_to_utf8_string(wkey);
	}
	else if(encoding == inival_googlecgiapi_encoding_utf8)
	{
		if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ckey.c_str(), -1, NULL, 0) == 0)
		{
			return;
		}
		ckey = std::string(key).substr(1, strlen(key) - 2);
		wkey = utf8_string_to_wstring(ckey);
	}
	else
	{
		return;
	}

	//検索除外
	try
	{
		if(std::regex_match(wkey, std::wregex(filter)))
		{
			return;
		}
	}
	catch(...)
	{
		return;
	}

	dwTimeout = wcstoul(timeout.c_str(), NULL, 0);
	if(dwTimeout == 0 || dwTimeout == ULONG_MAX)
	{
		dwTimeout = wcstoul(inival_def_timeout, NULL, 0);
	}

	_snwprintf_s(url, _TRUNCATE, L"%s", googlecgiapi_url_prefix);

	for(i = 0; i < ckey.size(); i++)
	{
		_snwprintf_s(pe, _TRUNCATE, L"%%%02X", (BYTE)ckey[i]);
		wcsncat_s(url, pe, _TRUNCATE);
	}

	std::string suffix = wstring_to_utf8_string(std::wstring(googlecgiapi_url_suffix));
	for(i = 0; i < suffix.size(); i++)
	{
		_snwprintf_s(pe, _TRUNCATE, L"%%%02X", (BYTE)suffix[i]);
		wcsncat_s(url, pe, _TRUNCATE);
	}

	hInet = InternetOpenW(useragent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(hInet != NULL)
	{
		InternetSetOptionW(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		InternetSetOptionW(hInet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		InternetSetOptionW(hInet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		hUrl = InternetOpenUrlW(hInet, url, NULL, 0, 0, 0);
		if(hUrl != NULL)
		{
			ZeroMemory(buf, sizeof(buf));
			InternetReadFile(hUrl, buf, BUFSIZE - 1, &bytesRead);
			InternetCloseHandle(hUrl);
		}
		InternetCloseHandle(hInet);
	}

	if(bytesRead == 0)
	{
		return;
	}

	wjson.assign(utf8_string_to_wstring(std::string(buf)));

	//エスケープシーケンス 制御文字
	wreg = L"\\\\[bfnrt]";
	wfmt = L"";
	wjson = std::regex_replace(wjson, wreg, wfmt);
	//エスケープシーケンス バックスラッシュ
	wreg = L"\\\\\\\\";
	wfmt = L"\\u005C";
	wjson = std::regex_replace(wjson, wreg, wfmt);
	//エスケープシーケンス スラッシュ
	wreg = L"\\\\/";
	wfmt = L"\\u002F";
	wjson = std::regex_replace(wjson, wreg, wfmt);
	//エスケープシーケンス ダブルクォーテーション
	wreg = L"\\\\\"";
	wfmt = L"\\u0022";
	//角括弧
	wjson = std::regex_replace(wjson, wreg, wfmt);
	wreg = L"\\[\\[.+,\\[(.+)\\]\\]\\]";
	wfmt = L"$1";
	wjson = std::regex_replace(wjson, wreg, wfmt);
	//各要素
	wreg = L"\".+?\"";
	wjson_tmp = wjson;
	wjson = L"/";
	while(std::regex_search(wjson_tmp, wres, wreg))
	{
		c = wres.str().substr(1, wres.str().size() - 2);
		if(c.find_first_of(L"/") != std::wstring::npos || c.find_first_of(L";") != std::wstring::npos)
		{
			wreg = L"/";
			wfmt = L"\\057";
			c = std::regex_replace(c, wreg, wfmt);
			wreg = L";";
			wfmt = L"\\073";
			c = std::regex_replace(c, wreg, wfmt);
			c = L"(concat \"" + c + L"\")";
		}
		//EUC-JIS-2004に変換できないとき候補に追加しない
		wjson_tmp = wres.suffix();
		dstsize = -1;
		if(encoding == inival_googlecgiapi_encoding_euc &&
			WideCharToEucJis2004(c.c_str(), NULL, NULL, &dstsize) == FALSE)
		{
			continue;
		}
		if(!annotation.empty())
		{
			c += L";" + annotation;
		}
		wjson += c + L"/";
	}
	//エスケープシーケンス Unicode
	wreg = L"\\\\u[0-9A-F]{4}";
	wjson_tmp = wjson;
	wjson.clear();
	while(std::regex_search(wjson_tmp, wres, wreg))
	{
		wjson.append(wres.prefix());
		WCHAR ch = (WCHAR)wcstoul(wres.str().substr(2).c_str(), NULL, 16);
		if(ch != L'\0')
		{
			wjson.push_back(ch);
		}
		wjson_tmp = wres.suffix();
	}
	wjson.append(wjson_tmp);
	//制御文字
	wreg = L"[\\x00-\\x19]";
	wfmt = L"";
	wjson = std::regex_replace(wjson, wreg, wfmt);

	wjson += L"\n";

	if(encoding == inival_googlecgiapi_encoding_euc)
	{
		s = wstring_to_euc_jis_2004_string(wjson);
	}
	else if(encoding == inival_googlecgiapi_encoding_utf8)
	{
		s = wstring_to_utf8_string(wjson);
	}
}
