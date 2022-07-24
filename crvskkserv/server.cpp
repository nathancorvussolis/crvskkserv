
#include "crvskkserv.h"
#include "utf8.h"

int make_serv_sock(SERVINFO *servinfo, int servinfonum)
{
	ADDRINFOW aiwHints;
	ADDRINFOW *paiwResult;
	ADDRINFOW *paiw;
	int i;
	BOOL use = TRUE;

	ZeroMemory(&aiwHints, sizeof(aiwHints));
	if (!serv_loopback)
	{
		aiwHints.ai_flags = AI_PASSIVE;
	}
	aiwHints.ai_family = AF_UNSPEC;
	aiwHints.ai_socktype = SOCK_STREAM;
	aiwHints.ai_protocol = IPPROTO_TCP;

	if (GetAddrInfoW(nullptr, serv_port, &aiwHints, &paiwResult) != 0)
	{
		return 0;
	}

	for (i = 0, paiw = paiwResult; (i < servinfonum) && (paiw != nullptr); paiw = paiw->ai_next)
	{
		servinfo[i].sock = socket(paiw->ai_family, paiw->ai_socktype, paiw->ai_protocol);
		if (servinfo[i].sock == INVALID_SOCKET)
		{
			continue;
		}

		if (setsockopt(servinfo[i].sock, SOL_SOCKET, SO_REUSEADDR,
			(const char *)&use, sizeof(use)) == SOCKET_ERROR)
		{
			disconnect(servinfo[i].sock);
			continue;
		}

		if (bind(servinfo[i].sock, paiw->ai_addr, (int)paiw->ai_addrlen) == SOCKET_ERROR)
		{
			disconnect(servinfo[i].sock);
			continue;
		}

		if (listen(servinfo[i].sock, 1) == SOCKET_ERROR)
		{
			disconnect(servinfo[i].sock);
			continue;
		}

		servinfo[i].live = TRUE;
		i++;
	}

	FreeAddrInfoW(paiwResult);
	return i;
}

void disconnect(SOCKET &sock)
{
	if (sock != INVALID_SOCKET)
	{
		shutdown(sock, SD_BOTH);
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

void comm(SOCKET &sock)
{
	CHAR rbuf[RBUFSIZE];
	std::string sbuf, ckey, s, res;
	int i, n;
	BOOL recvflag = TRUE;

	while (recvflag)
	{
		ZeroMemory(rbuf, sizeof(rbuf));
		n = recv(sock, rbuf, sizeof(rbuf) - 1, 0);
		if (n == SOCKET_ERROR || n <= 0)
		{
			disconnect(sock);
			return;
		}

		sbuf += rbuf;

		if (sbuf.empty())
		{
			disconnect(sock);
			return;
		}

		switch (sbuf.front())
		{
		case REQ_KEY:
		case REQ_CMP:
			if (rbuf[n - 1] == '\x20')
			{
				recvflag = FALSE;
			}
			break;
		case REQ_END:
		case REQ_VER:
		case REQ_HST:
		default:
			recvflag = FALSE;
			break;
		}
	}

	switch (sbuf.front())
	{
	case REQ_END:
		disconnect(sock);
		return;
		break;

	case REQ_KEY:
		if (sbuf.size() > 2)
		{
			ckey = sbuf.substr(1, sbuf.size() - 2);
			n = (int)vdicinfo.size();
			for (i = 0; i < n; i++)
			{
				s.clear();

				if (vdicinfo[i].path.compare(0, wcslen(INIVAL_SKKSERV), INIVAL_SKKSERV) == 0)
				{
					search_skkserv(vdicinfo[i], ckey, s);
				}
				else if (vdicinfo[i].path.compare(0, wcslen(INIVAL_GOOGLECGIAPI), INIVAL_GOOGLECGIAPI) == 0)
				{
					search_google_cgiapi(vdicinfo[i], ckey, s);
				}
				else
				{
					search_dictionary(vdicinfo[i], ckey, s);
				}

				if (!s.empty())
				{
					if (res.size() >= 2)
					{
						res.pop_back();	// '\n'
						res.pop_back(); // '/'
					}
					res += s;
				}
			}
		}

		if (res.empty())
		{
			res.push_back(REP_NOT_FOUND);
			res.push_back('\n');
		}
		else
		{
			res.insert(res.begin(), REP_FOUND);
		}
		break;

	case REQ_VER:
		res = resver;
		break;

	case REQ_HST:
	{
		CHAR host[NI_MAXHOST];
		SOCKADDR_STORAGE sa = {};
		int len = sizeof(sa);
		if (getsockname(sock, (LPSOCKADDR)&sa, &len) == 0)
		{
			if (getnameinfo((LPSOCKADDR)&sa, len, host, _countof(host), nullptr, 0, NI_NAMEREQD) == 0)
			{
				res += host;
				res += "/";
			}
			if (getnameinfo((LPSOCKADDR)&sa, len, host, _countof(host), nullptr, 0, NI_NUMERICHOST) == 0)
			{
				if (sa.ss_family == AF_INET6) res += "[";
				res += host;
				if (sa.ss_family == AF_INET6) res += "]";
				res += ":";
				res += WCTOU8(serv_port);
				res += "/\x20";
			}
		}
	}
	break;

	case REQ_CMP:
		res.push_back(REP_NOT_FOUND);
		res.push_back('\n');
		break;

	default:
		return;
		break;
	}

	if (send(sock, res.c_str(), (int)res.size(), 0) == SOCKET_ERROR)
	{
		disconnect(sock);
		return;
	}
}

void comm_thread(void *p)
{
	SERVINFO *pcinfo = (SERVINFO *)p;

	while (true)
	{
		comm(pcinfo->sock);
		if (pcinfo->sock == INVALID_SOCKET)
		{
			pcinfo->live = FALSE;
			break;
		}
	}
}

void listen_thread(void *p)
{
	SERVINFO *pservinfo = (SERVINFO *)p;
	SERVINFO cinfo[FD_SETSIZE] = {};
	SOCKADDR_STORAGE sockaddr = {};
	int i, sockaddrlen;

	for (i = 0; i < FD_SETSIZE; i++)
	{
		cinfo[i].live = FALSE;
		cinfo[i].sock = INVALID_SOCKET;
	}

	while (true)
	{
		for (i = 0; i < FD_SETSIZE; i++)
		{
			if (cinfo[i].sock == INVALID_SOCKET)
			{
				break;
			}
		}
		if (i == FD_SETSIZE)
		{
			Sleep(100);
			continue;
		}

		sockaddrlen = sizeof(sockaddr);
		cinfo[i].sock = accept(pservinfo->sock, (LPSOCKADDR)&sockaddr, &sockaddrlen);
		if (cinfo[i].sock == INVALID_SOCKET)
		{
			disconnect(pservinfo->sock);
			for (i = 0; i < FD_SETSIZE; i++)
			{
				disconnect(cinfo[i].sock);
			}
			for (i = 0; i < FD_SETSIZE; i++)
			{
				while (cinfo[i].live)
				{
					Sleep(10);
				}
			}
			pservinfo->live = FALSE;
			break;
		}
		else
		{
			cinfo[i].live = TRUE;
			_beginthread(comm_thread, 0, &cinfo[i]);
		}
	}
}
