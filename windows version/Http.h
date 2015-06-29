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

	//str_url:��ץȡ����ҳ��Ӧ��url
	//page_body_buf:��ҳ����Ϣ
	//page_head_buf:��ҳͷ��Ϣ
	//location:��ҳ�ض����Ӧ��url
	//sock:�׽���������
	int Fetch(string str_url, char **file_buf, char **file_head_buf, char **location, int *sock);

private:

	//ͨ��IO���ö�ȡ��ҳͷ��Ϣ
	int ReadHeader(int sock, char *header_ptr);

	//�����׽����ļ�������
	int CreateSocket(const char *host,int port);

	//�� CreateSocket���ã�ͨ��IO���õķ�ʽ���ӷ�����
	int NoBlockingConnect(int ,struct sockaddr * ,int);

	//���buf��ʣ��ռ��Ƿ����more�������ټ�more+1���ڴ�ռ�
	int CheckBufferSize(char **buf, int *buf_size, int more);
	
};

//extern pthread_mutex_t mutex;

#endif