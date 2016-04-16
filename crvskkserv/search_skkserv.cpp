
#include "crvskkserv.h"

void search_skkserv(DICINFO &dicinfo, const std::string &key, std::string &s)
{
	std::string res, ckey;
	CHAR rbuf[RBUFSIZE];
	int n;

	if(dicinfo.sock == INVALID_SOCKET)
	{
		connect_skkserv(dicinfo);
	}

	if(!get_skkserv_version(dicinfo.sock))
	{
		return;
	}

	ckey.push_back(REQ_KEY);
	ckey += key + "\x20";
	if(send(dicinfo.sock, ckey.c_str(), ckey.size(), 0) == SOCKET_ERROR)
	{
		disconnect(dicinfo.sock);
		return;
	}

	while(true)
	{
		ZeroMemory(rbuf, sizeof(rbuf));
		n = recv(dicinfo.sock, rbuf, sizeof(rbuf) - 1, 0);
		if(n == SOCKET_ERROR || n <= 0)
		{
			disconnect(dicinfo.sock);
			return;
		}

		res.append(rbuf);

		if(n <= _countof(rbuf) && rbuf[n - 1] == '\n')
		{
			break;
		}
	}

	if(res.size() > 2 && res.front() == REP_FOUND)
	{
		s = res.substr(1);
	}
}

void split_skkserv_path(DICINFO &dicinfo, std::wstring &host, std::wstring &port, std::wstring &timeout)
{
	size_t is, ie;

	is = 0;
	ie = dicinfo.path.find_first_of(INIVAL_SKKSERV, is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	host = dicinfo.path.substr(is, ie - is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	port = dicinfo.path.substr(is, ie - is);
	is = ie + 1;
	ie = dicinfo.path.find_first_of(INIVAL_SVR_SEP, is);
	timeout = dicinfo.path.substr(is, ie - is);
}

void connect_skkserv(DICINFO &dicinfo)
{
	ADDRINFOW aiwHints;
	ADDRINFOW *paiwResult;
	ADDRINFOW *paiw;
	u_long mode;
	timeval tv;
	fd_set fdw, fde;
	std::wstring host, port, timeout;
	DWORD dwTimeout;

	split_skkserv_path(dicinfo, host, port, timeout);

	dwTimeout = wcstoul(timeout.c_str(), nullptr, 0);
	if(dwTimeout == 0 || dwTimeout == ULONG_MAX)
	{
		dwTimeout = wcstoul(inival_def_timeout, nullptr, 0);
	}

	ZeroMemory(&aiwHints, sizeof(aiwHints));
	aiwHints.ai_family = AF_UNSPEC;
	aiwHints.ai_socktype = SOCK_STREAM;
	aiwHints.ai_protocol = IPPROTO_TCP;

	if(GetAddrInfoW(host.c_str(), port.c_str(), &aiwHints, &paiwResult) != 0)
	{
		return;
	}

	for(paiw = paiwResult; paiw != nullptr; paiw = paiw->ai_next)
	{
		dicinfo.sock = socket(paiw->ai_family, paiw->ai_socktype, paiw->ai_protocol); 
		if(dicinfo.sock == INVALID_SOCKET)
		{
			continue;
		}

		if(setsockopt(dicinfo.sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwTimeout, sizeof(dwTimeout)) == SOCKET_ERROR)
		{
			closesocket(dicinfo.sock);
			dicinfo.sock = INVALID_SOCKET;
			continue;
		}
		if(setsockopt(dicinfo.sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeout, sizeof(dwTimeout)) == SOCKET_ERROR)
		{
			closesocket(dicinfo.sock);
			dicinfo.sock = INVALID_SOCKET;
			continue;
		}

		mode = 1;
		ioctlsocket(dicinfo.sock, FIONBIO, &mode);

		if(connect(dicinfo.sock, paiw->ai_addr, (int)paiw->ai_addrlen) == SOCKET_ERROR)
		{
			if(WSAGetLastError() != WSAEWOULDBLOCK)
			{
				closesocket(dicinfo.sock);
				dicinfo.sock = INVALID_SOCKET;
				continue;
			}
		}

		mode = 0;
		ioctlsocket(dicinfo.sock, FIONBIO, &mode);

		tv.tv_sec = dwTimeout / 1000;
		tv.tv_usec = (dwTimeout % 1000) * 1000;

		FD_ZERO(&fdw);
		FD_ZERO(&fde);
		FD_SET(dicinfo.sock, &fdw);
		FD_SET(dicinfo.sock, &fde);

		select(0, nullptr, &fdw, &fde, &tv);
		if(FD_ISSET(dicinfo.sock, &fdw))
		{
			break;
		}

		disconnect(dicinfo.sock);
	}

	FreeAddrInfoW(paiwResult);
}

BOOL get_skkserv_version(SOCKET &sock)
{
	BOOL bRet = TRUE;
	int n;
	CHAR rbuf[RBUFSIZE];
	CHAR sbuf = REQ_VER;
	std::string res;

	if(send(sock, &sbuf, 1, 0) == SOCKET_ERROR)
	{
		disconnect(sock);
		bRet = FALSE;
	}
	else
	{
		while(true)
		{
			ZeroMemory(rbuf, sizeof(rbuf));
			n = recv(sock, rbuf, sizeof(rbuf) - 1, 0);
			if(n == SOCKET_ERROR || n <= 0)
			{
				disconnect(sock);
				bRet = FALSE;
				break;
			}

			if(rbuf[n - 1] == '\x20')
			{
				break;
			}
		}
	}

	return bRet;
}
