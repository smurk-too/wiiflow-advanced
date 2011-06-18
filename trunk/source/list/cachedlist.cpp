#include "cachedlist.hpp"
#include "gecko.h"

template <class T>
void CachedList<T>::Load(string path, string containing, findtitle_callback_t callback, void* callback_data)																/* Load All */
{
	gprintf("\nLoading files containing %s in %s\n", containing.c_str(), path.c_str());

	m_loaded = false;
	m_database = sfmt("%s/%s.db"/* "%s/%s_i.db" */, m_cacheDir.c_str(), (make_db_name(path)).c_str()/*,  disk guid (4) */);
	gprintf("Database file: %s\n", m_database.c_str());
	m_wbfsFS = strncasecmp(DeviceHandler::Instance()->PathToFSName(path.c_str()), "WBFS", 4) == 0;
	if(!m_wbfsFS)
	{
		struct stat filestat;
		struct stat cache;
		if(stat(path.c_str(), &filestat) == -1) return;
		m_update = (stat(m_database.c_str(), &cache) == -1 || filestat.st_mtime > cache.st_mtime);
	}
	
	safe_vector<string> pathlist;
	list.GetPaths(pathlist, containing, path, m_wbfsFS, &rcnt);

	if(m_update || m_wbfsFS)
	{
		gprintf("Calling list to update filelist\n");
		list.GetHeaders(pathlist, *this, callback, callback_data);
		
		// Load titles and custom_titles files
		m_loaded = true;
		Save();
	}
	else
	{
		CCache<T>(*this, m_database, &gcnt, rcnt, LOAD);
		if(rcnt != gcnt) 
		{
			gprintf("Real game count: %d Games in cache: %d. Rebuilding cache\n", rcnt, gcnt);
			list.GetHeaders(pathlist, *this);
			m_loaded = true;
			Save();
		}
		
	}
	m_loaded = true;
}

template <class T>
string CachedList<T>::make_db_name(string path)
{
	string buffer = path;
	buffer.replace(buffer.find(":/"), 2, "_");
	size_t find = buffer.find("/");
	while(find != string::npos)
	{
		buffer[find] = '_';
		find = buffer.find("/");
	}
	return buffer;
}

template class CachedList<string>;
template class CachedList<dir_discHdr>;
