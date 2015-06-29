#ifndef _HTTP_H
#define _HTTP_H

#include <map>
#include <string>
#include <winsock2.h>

using namespace std;


class CHttp
{

private:
	string url;
	int *sock;

public:
	CHttp();
	virtual ~CHttp();

	//str_url:待抓取的网页对应的url
	//page_body_buf:网页体信息
	//page_head_buf:网页头信息
	//location:网页重定向对应的url
	//sock:套接字描述符
	int Fetch(string str_url, char **file_buf, char **file_head_buf, char **location, int *sock);

private:

	//通过IO复用读取网页头信息
	int ReadHeader(int sock, char *header_ptr);

	//创建套接字文件描述符
	int CreateSocket(const char *host,int port);

	//被 CreateSocket调用，通过IO复用的方式连接服务器
	int NoBlockingConnect(int ,struct sockaddr * ,int);

	//检测buf的剩余空间是否大于more，不够再加more+1的内存空间
	int CheckBufferSize(char **buf, int *buf_size, int more);
	
};

//extern pthread_mutex_t mutex;

#endif