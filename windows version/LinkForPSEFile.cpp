#include "LinkForPSEFile.h"
#include <time.h>

extern map<string,string> map_cache_host_lookup;
extern vector<string> vector_parsed_links;

CLinkForPSEFile::CLinkForPSEFile()
{

}

CLinkForPSEFile::CLinkForPSEFile(string str):CFileEngine(str)
{

}

CLinkForPSEFile::~CLinkForPSEFile()
{
	ofs_file.close();
}

bool CLinkForPSEFile::Write(void *arg)
{
	if(!arg || !ofs_file)
		return false;

	if(vector_parsed_links.size() == 0)
		return false;

	file_arg *file_ptr = (file_arg *)arg;

	CUrl *iurl = file_ptr->url_ptr;
	CPage *ipage = file_ptr->page_ptr;

	char str_downroad_time[128];
	time_t t_date;

	memset(str_downroad_time,0,128);
	time(&t_date);
	strftime(str_downroad_time,128,"%a , %d %b %Y %H:%M:%S GMT",gmtime(&t_date));

	string links;
	vector<string>::iterator it;
	for(it=vector_parsed_links.begin();it!=vector_parsed_links.end();++it)
	{
		links = links + *it +"\n";
	}


	ofs_file<<"version: 1.0\n";
	if(ipage->location.length() == 0)//没有重定向
	{
		ofs_file<<"url: "<<iurl->origin_url;
	}
	else
	{
		ofs_file<<"url: "<<ipage->location;
		ofs_file<<"\norigion: "<<ipage->location;
	}

	ofs_file<<"\ndate: "<<str_downroad_time;//记录下载时间

	//保存IP地址
	if(map_cache_host_lookup.find(iurl->host_name) == map_cache_host_lookup.end())
	{
		ofs_file<<"\nip: "<<iurl->host_name;
	}
	else
	{
		ofs_file<<"\nip: "<<(*map_cache_host_lookup.find(iurl->host_name)).second;
	}

	ofs_file<<"\noutdegree: "<<vector_parsed_links.size();
	ofs_file<<"\nlength: "<<links.size() + ipage->header_length + 1
		<<"\n\n"<<ipage->header<<"\n";

	ofs_file<<links;
	ofs_file<<endl;

	return true;
}