
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
	if(fpidx != nullptr)
	{
		while(fread(&pos, sizeof(pos), 1, fpidx) == 1)
		{
			dicinfo.pos.push_back(pos);
		}
		fclose(fpidx);
		return;
	}
#endif

	_wfopen_s(&fpdic, dicinfo.path.c_str(), RB);
	if(fpdic == nullptr)
	{
		return;
	}

	pos = ftell(fpdic);
	while(true)
	{
		sbuf.clear();

		while((pb = fgets(buf, sizeof(buf), fpdic)) != nullptr)
		{
			sbuf += buf;

			if(!sbuf.empty() && sbuf.back() == '\n')
			{
				break;
			}
		}

		if(pb == nullptr)
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
			pidx = sbuf.find("\x20/");
			if(pidx != std::string::npos && pidx <= sbuf.size())
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
		if(fpidx != nullptr)
		{
			fwrite(&map_itr->second, sizeof(map_itr->second), 1, fpidx);
		}
#endif
	}

#if USEIDXFILE
	if(fpidx != nullptr)
	{
		fclose(fpidx);
	}
#endif
}

void search_dictionary(DICINFO &dicinfo, const std::string &key, std::string &s)
{
	FILE *fpdic;
	CHAR buf[DICBUFSIZE];
	std::string ckey, sbuf, kbuf, cbuf;
	long pos, left, mid, right;
	
	_wfopen_s(&fpdic, dicinfo.path.c_str(), RB);
	if(fpdic == nullptr)
	{
		return;
	}

	ckey = key + "\x20";

	left = 0;
	right = (long)dicinfo.pos.size() - 1;

	while(left <= right)
	{
		mid = left + (right - left) / 2;
		pos = dicinfo.pos[mid];
		fseek(fpdic, pos, SEEK_SET);
		
		sbuf.clear();
		kbuf.clear();
		cbuf.clear();

		while(fgets(buf, _countof(buf), fpdic) != nullptr)
		{
			sbuf += buf;

			if(!sbuf.empty() && sbuf.back() == '\n')
			{
				break;
			}
		}

		size_t cidx = sbuf.find("\x20/");
		if(cidx != std::wstring::npos && cidx < sbuf.size())
		{
			kbuf = sbuf.substr(0, cidx + 1);
			cbuf = sbuf.substr(cidx + 1);
		}

		int cmpkey = ckey.compare(kbuf);
		if(cmpkey == 0)
		{
			s = cbuf;
			break;
		}
		else if(cmpkey > 0)
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
