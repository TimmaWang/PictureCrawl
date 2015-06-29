#include "ISAMFile.h"
#include "Page.h"
#include "Url.h"

CISAMFile::CISAMFile()
{
	data_file_ptr = NULL;
	index_file_ptr = NULL;
}

CISAMFile::CISAMFile(string str):CFileEngine(str)
{
	data_file_ptr = NULL;
	index_file_ptr = NULL;
}

CISAMFile::CISAMFile(string data_str,string index_str)
{
	data_file_ptr = NULL;
	index_file_ptr = NULL;

	str = data_str;
	index_file_name = index_str;

	data_file_ptr = fopen(DATA_FILE_NAME.c_str(),"a");
	if(data_file_ptr == NULL)
	{
		cout<<"can not open the file web_data"<<endl;
		return;
	}

	index_file_ptr = fopen(INDEX_FILE_NAME.c_str(),"a");
	if(index_file_ptr == NULL)
	{
		cout<<"can not open the file windex"<<endl;
		return;
	}

}


CISAMFile::~CISAMFile()
{

}

bool CISAMFile::Open(string data_str,string index_str)
{
	str = data_str;
	index_file_name = index_str;

	
	data_file_ptr = fopen(DATA_FILE_NAME.c_str(),"at");
	if(data_file_ptr == NULL)
	{
		return false;
	}

	index_file_ptr = fopen(INDEX_FILE_NAME.c_str(),"at");
	if(index_file_ptr == NULL)
	{
		return false;
	}

	return true;
}


void CISAMFile::Close()
{
	fclose(data_file_ptr);
	fclose(index_file_ptr);
}

bool CISAMFile::Write(void *arg)
{
	if(!arg || !data_file_ptr ||!index_file_ptr)
		return false;

	file_arg *file_ptr = (file_arg *)arg;
	CUrl *iurl = file_ptr->url_ptr;
	CPage *ipage = file_ptr->page_ptr;

	const char *url = NULL;
	const char *buffer = NULL;

	if(ipage->location.length() ==0)
	{
		url = iurl->origin_url.c_str();
	}
	else
	{
		url = ipage->location.c_str();
	}

	buffer = ipage->body_content.c_str();
	int len = ipage->body_content.length();

	int offset = ftell(data_file_ptr);//获得当前的文件位置

	fprintf(index_file_ptr,"%10d",offset);
	fprintf(index_file_ptr,"%256s\n",url);
	fflush(index_file_ptr);//刷新文件

	fwrite(buffer,1,len,data_file_ptr);

	//write 15 spaces in file
	for(int i=0;i<25;i++)
	{
		fputc(0,data_file_ptr);
	}

	//write 3 "1" in file
	fputc(1,data_file_ptr);
	fputc(1,data_file_ptr);
	fputc(1,data_file_ptr);

	//write [url] in file
	fputc(91,data_file_ptr);
	fwrite(buffer,1,strlen(url),data_file_ptr);
	fputc(93,data_file_ptr);

	for(int i=0;i<25;i++)
	{
		fputc(0,data_file_ptr);
	}

	fflush(data_file_ptr);

	return true;
}