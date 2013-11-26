
#include "stdafx.h"
#include "crvskkserv.h"

int make_serv_sock(SERVINFO *servinfo, int servinfonum)
{
	ADDRINFOW aiwHints;
	ADDRINFOW *paiwResult;
	ADDRINFOW *paiw;
	int i;
	BOOL use = TRUE;

	ZeroMemory(&aiwHints, sizeof(aiwHints));
	if(!serv_loopback)
	{
		aiwHints.ai_flags = AI_PASSIVE;
	}
	aiwHints.ai_family = AF_UNSPEC;
	aiwHints.ai_socktype = SOCK_STREAM;
	aiwHints.ai_protocol = IPPROTO_TCP;

	if(GetAddrInfoW(NULL, serv_port, &aiwHints, &paiwResult) != 0)
	{
		return 0;
	}

	for(i = 0, paiw = paiwResult; i < servinfonum, paiw != NULL; paiw = paiw->ai_next)
	{
		servinfo[i].sock = socket(paiw->ai_family, paiw->ai_socktype, paiw->ai_protocol); 
		if(servinfo[i].sock == INVALID_SOCKET)
		{
			continue;
		}

		if(setsockopt(servinfo[i].sock, SOL_SOCKET, SO_REUSEADDR,
			(const char *)&use, sizeof(use)) == SOCKET_ERROR)
		{
			disconnect(servinfo[i].sock);
			continue;
		}

		if(bind(servinfo[i].sock, paiw->ai_addr, (int)paiw->ai_addrlen) == SOCKET_ERROR)
		{
			disconnect(servinfo[i].sock);
			continue;
		}

		if(listen(servinfo[i].sock, 1) == SOCKET_ERROR)
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
	if(sock != INVALID_SOCKET)
	{
		shutdown(sock, SD_BOTH);
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
}

void comm(SOCKET &sock)
{
	CHAR key[KEYSIZE];
	std::string s, res;
	int i, n;

	memset(key, 0, sizeof(key));

	n = recv(sock, key, sizeof(key) - 1, 0);
	if(n == SOCKET_ERROR || n == 0)
	{
		disconnect(sock);
		return;
	}

	switch(key[0])
	{
	case REQ_END:
		disconnect(sock);
		return;
		break;
	case REQ_KEY:
		n = (int)vdicinfo.size();
		for(i = 0; i < n; i++)
		{
			if(vdicinfo[i].path.compare(0, wcslen(INIVAL_SKKSERV), INIVAL_SKKSERV) == 0)
			{
				search_skkserv(vdicinfo[i], key, s);
			}
			else if(vdicinfo[i].path.compare(0, wcslen(INIVAL_GOOGLECGIAPI), INIVAL_GOOGLECGIAPI) == 0)
			{
				search_google_cgiapi(vdicinfo[i], key, s);
			}
			else
			{
				search_dictionary(vdicinfo[i], &key[1], s);
			}

			if(!s.empty())
			{
				if(res.size() >= 2)
				{
					res.pop_back();	// '\n'
					res.pop_back(); // '/'
				}
				res += s;
			}
		}
		if(res.empty())
		{
			res.push_back(REP_NG);
			res.push_back('\n');
		}
		else
		{
			res.insert(res.begin(), REP_OK);
		}
		break;
	case REQ_VER:
	case REQ_ADR:
		res = resver;
		break;
	default:
		res.push_back(REP_NG);
		res.push_back('\n');
		break;
	}

	if(send(sock, res.c_str(), (int)res.size(), 0) == SOCKET_ERROR)
	{
		disconnect(sock);
		return;
	}
}

void comm_thread(void *p)
{
	SERVINFO *pcinfo = (SERVINFO *)p;

	while(true)
	{
		comm(pcinfo->sock);
		if(pcinfo->sock == INVALID_SOCKET)
		{
			pcinfo->live = FALSE;
			break;
		}
	}
}

void listen_thread(void *p)
{
	SERVINFO *pservinfo = (SERVINFO *)p;
	SERVINFO cinfo[FD_SETSIZE];
	SOCKADDR_STORAGE sockaddr;
	int i, sockaddrlen;

	for(i = 0; i < FD_SETSIZE; i++)
	{
		cinfo[i].live = FALSE;
		cinfo[i].sock = INVALID_SOCKET;
	}

	while(true)
	{
		for(i = 0; i < FD_SETSIZE; i++)
		{
			if(cinfo[i].sock == INVALID_SOCKET)
			{
				break;
			}
		}
		if(i == FD_SETSIZE)
		{
			Sleep(100);
			continue;
		}

		sockaddrlen = sizeof(sockaddr);
		cinfo[i].sock = accept(pservinfo->sock, (LPSOCKADDR)&sockaddr, &sockaddrlen);
		if(cinfo[i].sock == INVALID_SOCKET)
		{
			disconnect(pservinfo->sock);
			for(i = 0; i < FD_SETSIZE; i++)
			{
				disconnect(cinfo[i].sock);
			}
			for(i = 0; i < FD_SETSIZE; i++)
			{
				while(cinfo[i].live)
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
