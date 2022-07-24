
#include "crvskkserv.h"

void init_search_dictionary(DICINFO &dicinfo)
{
	CHAR buf[DICBUFSIZE];
	std::string sbuf;
	int okuri = 0; // default okuri-nasi
	MAP map;

	FILE *fpdic = nullptr;
	_wfopen_s(&fpdic, dicinfo.path.c_str(), modeRB);
	if (fpdic == nullptr)
	{
		return;
	}

	long pos = ftell(fpdic);

	while (true)
	{
		sbuf.clear();

		while (fgets(buf, sizeof(buf), fpdic) != nullptr)
		{
			sbuf += buf;

			if (!sbuf.empty() && sbuf.back() == '\n')
			{
				break;
			}
		}

		if (ferror(fpdic) != 0)
		{
			fclose(fpdic);
			return;
		}

		if (sbuf.empty())
		{
			break;
		}

		if (sbuf.compare(EntriesAri) == 0)
		{
			okuri = 1;
		}
		else if (sbuf.compare(EntriesNasi) == 0)
		{
			okuri = 0;
		}
		else if (okuri != -1)
		{
			size_t pidx = sbuf.find("\x20/");
			if (pidx != std::string::npos && pidx <= sbuf.size())
			{
				map.insert(PAIR(sbuf.substr(0, pidx), pos));
			}
		}

		pos = ftell(fpdic);
	}

	fclose(fpdic);

	dicinfo.pos.clear();
	for (auto map_itr = map.begin(); map_itr != map.end(); map_itr++)
	{
		dicinfo.pos.push_back(map_itr->second);
	}
}

void search_dictionary(DICINFO &dicinfo, const std::string &key, std::string &s)
{
	CHAR buf[DICBUFSIZE];
	std::string ckey, sbuf, kbuf, cbuf;

	FILE *fpdic = nullptr;
	_wfopen_s(&fpdic, dicinfo.path.c_str(), modeRB);
	if (fpdic == nullptr)
	{
		return;
	}

	ckey = key + "\x20";

	long left = 0;
	long right = (long)dicinfo.pos.size() - 1;

	while (left <= right)
	{
		long mid = left + (right - left) / 2;
		long pos = dicinfo.pos[mid];
		fseek(fpdic, pos, SEEK_SET);

		sbuf.clear();
		kbuf.clear();
		cbuf.clear();

		while (fgets(buf, _countof(buf), fpdic) != nullptr)
		{
			sbuf += buf;

			if (!sbuf.empty() && sbuf.back() == '\n')
			{
				break;
			}
		}

		if (ferror(fpdic) != 0)
		{
			fclose(fpdic);
			return;
		}

		if (sbuf.empty())
		{
			break;
		}

		// LF
		if (sbuf.back() != '\n')
		{
			sbuf.push_back('\n');
		}

		size_t cidx = sbuf.find("\x20/");
		if (cidx != std::wstring::npos && cidx < sbuf.size())
		{
			kbuf = sbuf.substr(0, cidx + 1);
			cbuf = sbuf.substr(cidx + 1);
		}

		int cmpkey = ckey.compare(kbuf);
		if (cmpkey == 0)
		{
			s = cbuf;
			break;
		}
		else if (cmpkey > 0)
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
