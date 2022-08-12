
#include "crvskkserv.h"
#include "eucjis2004.h"
#include "eucjp.h"
#include "utf8.h"
#include "picojson.h"

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
	WCHAR url[INTERNET_MAX_URL_LENGTH];
	std::wstring filter, annotation, timeout, encoding;
	std::wstring wkey;
	std::string ckey;

	//接続情報取得
	split_google_cgiapi_path(dicinfo, filter, annotation, timeout, encoding);

	//文字コードチェック、UTF-8変換
	if (encoding == inival_googlecgiapi_encoding_euc)
	{
		wkey = eucjis2004_string_to_wstring(key);
		if (wkey.empty())
		{
			return;
		}
		ckey = wstring_to_utf8_string(wkey);
	}
	else if (encoding == inival_googlecgiapi_encoding_utf8)
	{
		ckey = key;
		wkey = utf8_string_to_wstring(ckey);
		if (wkey.empty())
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
		if (std::regex_match(wkey, std::wregex(filter)))
		{
			return;
		}
	}
	catch (...)
	{
		return;
	}

	DWORD dwTimeout = wcstoul(timeout.c_str(), nullptr, 0);
	if (dwTimeout == 0 || dwTimeout == ULONG_MAX)
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

	HINTERNET hInet = InternetOpenW(useragent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (hInet != nullptr)
	{
		InternetSetOptionW(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		InternetSetOptionW(hInet, INTERNET_OPTION_SEND_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
		InternetSetOptionW(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));

		HINTERNET hUrl = InternetOpenUrlW(hInet, url, nullptr, 0, 0, 0);
		if (hUrl != nullptr)
		{
			CHAR rbuf[RBUFSIZE];

			while (true)
			{
				DWORD bytesRead = 0;
				ZeroMemory(rbuf, sizeof(rbuf));
				BOOL retRead = InternetReadFile(hUrl, rbuf, sizeof(rbuf) - 1, &bytesRead);
				if (retRead)
				{
					if (bytesRead == 0) break;
				}
				else
				{
					InternetCloseHandle(hUrl);
					InternetCloseHandle(hInet);
					return;
				}

				res.append(rbuf, bytesRead);
			}
			InternetCloseHandle(hUrl);
		}
		InternetCloseHandle(hInet);
	}

	std::vector<std::wstring> vws;

	// Example
	// [["こうし",["講師","格子","行使","公私","こうし"]]]

	picojson::value json_value;
	const std::string json_err = picojson::parse(json_value, res);

	if (json_err.empty())
	{
		if (json_value.is<picojson::array>())
		{
			const picojson::array &json_array_1 = json_value.get<picojson::array>();

			for (const auto &json_value_1 : json_array_1)
			{
				if (json_value_1.is<picojson::array>())
				{
					const picojson::array &json_array_2 = json_value_1.get<picojson::array>();

					bool key_hit = false;

					for (const auto &json_value_2 : json_array_2)
					{
						if (json_value_2.is<std::string>())
						{
							// key
							key_hit = (json_value_2.get<std::string>() == wstring_to_utf8_string(wkey));
						}
						else if (json_value_2.is<picojson::array>())
						{
							const picojson::array &json_array_3 = json_value_2.get<picojson::array>();

							for (const auto &json_value_3 : json_array_3)
							{
								if (key_hit && json_value_3.is<std::string>())
								{
									// entry
									vws.push_back(utf8_string_to_wstring(json_value_3.get<std::string>()));
								}
							}
						}
					}
				}
			}
		}
	}

	std::wregex wreg;
	std::wstring wfmt;
	std::wsmatch wres;

	std::wstring ws = L"/";
	for (std::wstring &c : vws)
	{
		// 制御文字
		wreg = L"[\\x00-\\x19]";
		wfmt = L"";
		c = std::regex_replace(c, wreg, wfmt);

		if (c.find_first_of(L"/") != std::wstring::npos ||
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

		ws += c;
		if (!annotation.empty())
		{
			ws += L";" + annotation;
		}
		ws += L"/";
	}
	ws += L"\n";

	std::wstring ws_tmp = ws;
	wreg = L"/[^/]+";
	while (std::regex_search(ws_tmp, wres, wreg))
	{
		if (encoding == inival_googlecgiapi_encoding_euc)
		{
			std::string c = wstring_to_eucjis2004_string(wres.str());
			if (c.empty())
			{
				c = wstring_to_eucjp_string(wres.str());
			}
			s += c;
		}
		else if (encoding == inival_googlecgiapi_encoding_utf8)
		{
			s += wstring_to_utf8_string(wres.str());
		}
		ws_tmp = wres.suffix();
	}
}
