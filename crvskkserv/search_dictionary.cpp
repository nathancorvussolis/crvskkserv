
#include "crvskkserv.h"

#define USEIDXFILE	0

void init_search_dictionary(DICINFO &dicinfo)
{
	CHAR buf[DICBUFSIZE];
	LPSTR pb;
	std::string sbuf;
	FILE *fpdic;
	long pos;
	int okuri = -1;
	MAP map;
	size_t pidx;
#if USEIDXFILE
	FILE *fpidx;
	wchar_t idxpath[MAX_PATH];

	_snwprintf_s(idxpath, _TRUNCATE, L"%s.idx", dicinfo.path.c_str());

	_wfopen_s(&fpidx, idxpath, RB);
	if(fpidx != NULL)
	{
		while(fread(&pos, 4, 1, fpidx) == 1)
		{
			dicinfo.pos.push_back(pos);
		}
		fclose(fpidx);
		return;
	}
#endif

	_wfopen_s(&fpdic, dicinfo.path.c_str(), RB);
	if(fpdic == NULL)
	{
		return;
	}

	pos = ftell(fpdic);
	while(true)
	{
		sbuf.clear();

		while((pb = fgets(buf, sizeof(buf), fpdic)) != NULL)
		{
			sbuf.append(buf);

			if(!sbuf.empty() && sbuf.back() == '\n')
			{
				break;
			}
		}

		if(pb == NULL)
		{
			break;
		}

		if(sbuf.compare(EntriesAri) == 0)
		{
			okuri = 1;
		}
		else if(sbuf.compare(EntriesNasi) == 0)
		{
			okuri = 0;
		}
		else if(okuri != -1)
		{
			if((pidx = sbuf.find_first_of('\x20')) != std::string::npos)
			{
				map.insert(PAIR(sbuf.substr(0, pidx), pos));
			}
		}

		pos = ftell(fpdic);
	}

	fclose(fpdic);

#if USEIDXFILE
	_wfopen_s(&fpidx, idxpath, WB);
#endif

	dicinfo.pos.clear();
	for(auto map_itr = map.begin(); map_itr != map.end(); map_itr++)
	{
		dicinfo.pos.push_back(map_itr->second);

#if USEIDXFILE
		if(fpidx != NULL)
		{
			fwrite(&map_itr->second, sizeof(map_itr->second), 1, fpidx);
		}
#endif
	}

#if USEIDXFILE
	if(fpidx != NULL)
	{
		fclose(fpidx);
	}
#endif
}

void search_dictionary(DICINFO &dicinfo, const std::string &key, std::string &s)
{
	FILE *fpdic;
	CHAR buf[DICBUFSIZE];
	std::string sbuf, ckey;
	long pos, left, mid, right;
	int comp;
	size_t pidx;
	
	_wfopen_s(&fpdic, dicinfo.path.c_str(), RB);
	if(fpdic == NULL)
	{
		return;
	}

	ckey = key + "\x20";

	left = 0;
	right = dicinfo.pos.size() - 1;

	while(left <= right)
	{
		mid = (left + right) / 2;
		pos = dicinfo.pos.at(mid);

		fseek(fpdic, pos, SEEK_SET);
		memset(buf, 0, sizeof(buf));
		
		sbuf.clear();

		while(fgets(buf, _countof(buf), fpdic) != NULL)
		{
			sbuf += buf;

			if(!sbuf.empty() && sbuf.back() == '\n')
			{
				break;
			}
		}

		comp = strncmp(ckey.c_str(), sbuf.c_str(), ckey.size());
		if(comp == 0)
		{
			if((pidx = sbuf.find_first_of('\x20')) != std::string::npos)
			{
				if((pidx = sbuf.find_first_of('/', pidx)) != std::string::npos)
				{
					s = sbuf.substr(pidx);
				}
			}
			break;
		}
		else if(comp > 0)
		{
			left = mid + 1;
		}
		else
		{
			right = mid - 1;
		}
	}

	fclose(fpdic);
}
