﻿
#include "crvskkserv.h"
#include "eucjis2004.h"
#include "eucjp.h"
#include "utf8.h"

// https://www.google.co.jp/ime/cgiapi.html

LPCWSTR useragent = L"Mozilla/5.0 (compatible; " APP_TITLE L"/" APP_VERSION L")";

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

void search_google_cgiapi(DICINFO &dicinfo, const std::string &key, std::string &s)
{
	HINTERNET hInet;
	HINTERNET hUrl;
	WCHAR url[INTERNET_MAX_URL_LENGTH];
	std::wstring filter, annotation, timeout, encoding;
	std::wstring wkey;
	std::string ckey;

	//接続情報取得
	split_google_cgiapi_path(dicinfo, filter, annotation, timeout, encoding);

	//文字コードチェック、UTF-8変換
	if(encoding == inival_googlecgiapi_encoding_euc)
	{
		wkey = eucjis2004_string_to_wstring(key);
		if(wkey.empty())
		{
			return;
		}
		ckey = wstring_to_utf8_string(wkey);
	}
	else if(encoding == inival_googlecgiapi_encoding_utf8)
	{
		ckey = key;
		wkey = utf8_string_to_wstring(ckey);
		if(wkey.empty())
		{
			return;
		}
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

	DWORD dwTimeout = wcstoul(timeout.c_str(), nullptr, 0);
	if(dwTimeout == 0 || dwTimeout == ULONG_MAX)
	{
		dwTimeout = wcstoul(inival_def_timeout, nullptr, 0);
	}

	{
		_snwprintf_s(url, _TRUNCATE, L"%s", googlecgiapi_url_prefix);

		WCHAR pe[4];

		for (size_t i = 0; i < ckey.size(); i++)
		{
			_snwprintf_s(pe, _TRUNCATE, L"%%%02X", (BYTE)ckey[i]);
			wcsncat_s(url, pe, _TRUNCATE);
		}

		std::string suffix = wstring_to_utf8_string(std::wstring(googlecgiapi_url_suffix));
		for (size_t i = 0; i < suffix.size(); i++)
		{
			_snwprintf_s(pe, _TRUNCATE, L"%%%02X", (BYTE)suffix[i]);
			wcsncat_s(url, pe, _TRUNCATE);
		}
	}

	std::string res;

	hInet = InternetOpenW(useragent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if(hInet != nullptr)
	{
		InternetSetOptionW(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		InternetSetOptionW(hInet, INTERNET_OPTION_SEND_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		InternetSetOptionW(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));

		hUrl = InternetOpenUrlW(hInet, url, nullptr, 0, 0, 0);
		if(hUrl != nullptr)
		{
			CHAR rbuf[RBUFSIZE];

			while(true)
			{
				DWORD bytesRead = 0;
				ZeroMemory(rbuf, sizeof(rbuf));
				BOOL retRead = InternetReadFile(hUrl, rbuf, sizeof(rbuf) - 1, &bytesRead);
				if(retRead)
				{
					if(bytesRead == 0) break;
				}
				else
				{
					InternetCloseHandle(hUrl);
					InternetCloseHandle(hInet);
					return;
				}

				res.append(rbuf);
			}
			InternetCloseHandle(hUrl);
		}
		InternetCloseHandle(hInet);
	}

	std::wstring wjson = utf8_string_to_wstring(res);

	std::wregex wreg;
	std::wstring wfmt;
	std::wsmatch wres;

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
	wjson = std::regex_replace(wjson, wreg, wfmt);

	//角括弧
	wreg = L"\\[\\[.+,\\[(.+)\\]\\]\\]";
	wfmt = L"$1";
	wjson = std::regex_replace(wjson, wreg, wfmt);

	//各要素
	wreg = L"\".+?\"";
	std::wstring wjson_tmp = wjson;
	wjson = L"/";
	while(std::regex_search(wjson_tmp, wres, wreg))
	{
		std::wstring c = wres.str().substr(1, wres.str().size() - 2);
		if(c.find_first_of(L"/") != std::wstring::npos ||
			c.find_first_of(L";") != std::wstring::npos)
		{
			wreg = L"/";
			wfmt = L"\\057";
			c = std::regex_replace(c, wreg, wfmt);
			wreg = L";";
			wfmt = L"\\073";
			c = std::regex_replace(c, wreg, wfmt);
			c = L"(concat \"" + c + L"\")";
		}

		if(!annotation.empty())
		{
			c += L";" + annotation;
		}
		wjson += c + L"/";

		wjson_tmp = wres.suffix();
	}

	//エスケープシーケンス Unicode
	wreg = L"\\\\u[0-9A-F]{4}";
	wjson_tmp = wjson;
	wjson.clear();
	while(std::regex_search(wjson_tmp, wres, wreg))
	{
		wjson.append(wres.prefix());
		WCHAR ch = (WCHAR)wcstoul(wres.str().substr(2).c_str(), nullptr, 16);
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

	wjson_tmp = wjson;
	wreg = L"/[^/]+";
	while(std::regex_search(wjson_tmp, wres, wreg))
	{
		if(encoding == inival_googlecgiapi_encoding_euc)
		{
			std::string c = wstring_to_eucjis2004_string(wres.str());
			if (c.empty())
			{
				c = wstring_to_eucjp_string(wres.str());
			}
			s += c;
		}
		else if(encoding == inival_googlecgiapi_encoding_utf8)
		{
			s += wstring_to_utf8_string(wres.str());
		}
		wjson_tmp = wres.suffix();
	}
}
