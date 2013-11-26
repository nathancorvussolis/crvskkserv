
#include "stdafx.h"
#include "crvskkserv.h"

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
			LPSTR utf8 = new CHAR[len + 1];
			WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, utf8, len + 1, NULL, NULL);
			ret = utf8;
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
			LPWSTR wcs = new WCHAR[len + 1];
			MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, wcs, len + 1);
			ret = wcs;
			delete[] wcs;
		}
		catch(...)
		{
		}
	}

	return ret;
}

void split_google_cgiapi_path(DICINFO &dicinfo, std::wstring &filter, std::wstring &comment, std::wstring &timeout)
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
}

void search_google_cgiapi(DICINFO &dicinfo, LPCSTR key, std::string &s)
{
	HINTERNET hInet;
	HINTERNET hUrl;
	WCHAR url[INTERNET_MAX_URL_LENGTH];
	CHAR buf[BUFSIZE];
	DWORD bytesRead;
	WCHAR pe[4];
	size_t i;
	std::string json;
	std::regex re;
	std::string fmt;
	std::wstring filter, annotation, timeout;
	std::string c_annotation;
	DWORD dwTimeout;

	if(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, key, -1, NULL, 0) == 0)
	{
		return;
	}

	split_google_cgiapi_path(dicinfo, filter, annotation, timeout);

	std::wstring wkey = utf8_string_to_wstring(std::string(key).substr(1, strlen(key) - 2));
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

	c_annotation = wstring_to_utf8_string(annotation);

	dwTimeout = wcstoul(timeout.c_str(), NULL, 0);
	if(dwTimeout == 0 || dwTimeout == ULONG_MAX)
	{
		dwTimeout = wcstoul(inival_def_timeout, NULL, 0);
	}

	_snwprintf_s(url, _TRUNCATE, L"%s", googlecgiapi_url_prefix);

	for(i = 1; key[i] != '\0' && key[i] != '\x20'; i++)
	{
		_snwprintf_s(pe, _TRUNCATE, L"%%%02X", (BYTE)key[i]);
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

	json.assign(buf);
	json = std::regex_replace(json, std::regex("\\n"), std::string(""));
	json = std::regex_replace(json, std::regex("\\[\\[.+,\\[(.+)\\]\\]\\]"), std::string("$1"));
	json = std::regex_replace(json, std::regex("\\\""), std::string(""));
	json = std::regex_replace(json, std::regex(","), (c_annotation.empty() ? std::string("") : (";" + c_annotation + "/")));
	json = "/" + json;
	if(!c_annotation.empty())
	{
		json += ";" + c_annotation;
	}
	json += "/\n";
	s = json;
}
