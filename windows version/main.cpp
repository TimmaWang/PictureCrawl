#include "FileName.h"
#include "Crawl.h"
#include <WinSock2.h>

 WSADATA wsaData;
 WORD wVersionRequested=MAKEWORD(2,0);
 int nresult = WSAStartup(wVersionRequested,&wsaData);

int main()
{
	string input_file_name("seed.txt");
	string output_file_name("crawled_links.txt");
	CCrawl crawl(input_file_name.c_str(),output_file_name.c_str());
	
	crawl.DoCrawl();
}



 
