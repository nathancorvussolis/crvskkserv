
#include "crvskkserv.h"

#define USEIDXFILE	0

void init_search_dictionary(DICINFO &dicinfo)
{
	CHAR buf[BUFSIZE];
	FILE *fpdic;
	long pos;
	int okuri = -1;
	MAP map;
	MAP::iterator map_itr;
	char *p;
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
	while(fgets(buf, sizeof(buf), fpdic) != NULL)
	{
		if(strcmp(EntriesAri, buf) == 0)
		{
			okuri = 1;
		}
		else if(strcmp(EntriesNasi, buf) == 0)
		{
			okuri = 0;
		}
		else if(okuri != -1)
		{
			p = strchr(buf, '\x20');
			if(p != NULL)
			{
				*p = '\0';
				map.insert(PAIR(buf, pos));
			}
		}
		pos = ftell(fpdic);
	}

	fclose(fpdic);

#if USEIDXFILE
	_wfopen_s(&fpidx, idxpath, WB);
#endif

	dicinfo.pos.clear();
	for(map_itr = map.begin(); map_itr != map.end(); map_itr++)
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

void search_dictionary(DICINFO &dicinfo, LPCSTR key, std::string &s)
{
	CHAR buf[BUFSIZE];
	FILE *fpdic;
	long pos, left, mid, right;
	int comp;
	char *p;

	s.clear();

	_wfopen_s(&fpdic, dicinfo.path.c_str(), RB);
	if(fpdic == NULL)
	{
		return;
	}

	left = 0;
	right = dicinfo.pos.size() - 1;

	while(left <= right)
	{
		mid = (left + right) / 2;
		pos = dicinfo.pos.at(mid);

		fseek(fpdic, pos, SEEK_SET);
		memset(buf, 0, sizeof(buf));
		fgets(buf, _countof(buf), fpdic);

		comp = strncmp(key, buf, strlen(key));
		if(comp == 0)
		{
			if((p = strchr(buf, '\x20')) != NULL)
			{
				if((p = strchr(p, '/')) != NULL)
				{
					s = p;
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
