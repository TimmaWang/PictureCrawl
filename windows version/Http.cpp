#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <fcntl.h>

#include "Http.h"
#include "Url.h"
#include "Page.h"
#include "StrFunction.h"
#include "PSEDefine.h"



char *user_agent = NULL;
int  timeout = DEFAULT_TIMEOUT;
int  hide_agent = 0;

CHttp::CHttp()
{

}

CHttp::~CHttp()
{

}


/*****************************************************************
** Function name: Fetch
** Success :������ҳ���ֽ���
** Fail : ����  -1 ���ִ���
          ����  -2 ��IP������Χ��
		  ����  -3 ��Ч��������
		  ����  -4 image/text����
		  ����  -300 ��ҳ�ض���
*****************************************************************/
int CHttp::Fetch(string str_url, char **file_buf, char **file_head_buf, char **location, int *sock_ptr)
{
	char *tmp,*url,*request_buffer,*page_buffer;
	const char *host,*path;
	int sock, bytes_read = 0,buffer_size = REQUEST_BUF_SIZE;
	int ret, tmp_size,select_ret;
	int port = 80;

	if(str_url.empty())
	{
		cout<<"the url is null."<<endl;
		return -1;
	}

	//����url
	url = strdup(str_url.c_str());

	if(url == NULL)
	{
		cout<<"error , strdup in fetch."<<endl;
		return -1;
	}

	//parse the url
	CUrl u;
	if(u.ParseUrl(url) == false)
	{
		cout<<"parse url error in fetch ."<<endl;
		return -1;
	}

	host = u.host_name.c_str();
	path = u.request_path.c_str();

	if(u.port_number > 0)
		port = u.port_number;

	//����http������

	request_buffer = (char *)malloc(REQUEST_BUF_SIZE);

	if(request_buffer == NULL)
	{
		if(url)
		{
			free(url);
			url = NULL;
		}

		cout<<"can not molloc the enough memory for the request."<<endl;
		return -1;
	}

	request_buffer[0] = 0;

	//˵��������Ǹ�Ŀ¼�µ���ҳ
	if(strlen(path) < 1)
	{
		tmp_size = strlen("GET /") + strlen(HTTP_VERSION) + 2;
		//=================================================�޸Ĺ���request
		strcat(request_buffer, "GET ");
		strcat(request_buffer, "/");
		strcat(request_buffer, " HTTP/1.0\r\n");
		
		//=================================================
		

	/*	if(CheckBufferSize(&request_buffer , &buffer_size , tmp_size) ||
			_snprintf(request_buffer,buffer_size,"GET / %s\r\n",HTTP_VERSION) < 0)//
		{
			if(url)
			{
				free(url);
				url= NULL;
			}

			if(request_buffer)
			{
				free(request_buffer);
				request_buffer= NULL;
			}

			cout<<"��Ŀ¼ check buffer size error."<<endl;
			return -1;
		}*/
	}
	else//�������·�����Ǹ�Ŀ¼ʱ
	{
		tmp_size = strlen("GET ") + strlen(path) + strlen(HTTP_VERSION) + 4;
		strcat(request_buffer, "GET ");
		strcat(request_buffer, path);
		strcat(request_buffer, " HTTP/1.0\r\n");

		/*if(CheckBufferSize(&request_buffer , &buffer_size , tmp_size) ||
			_snprintf(request_buffer,buffer_size,"GET %s %s\r\n",path,HTTP_VERSION) < 0)//
		{
			if(url)
			{
				free(url);
				url= NULL;
			}

			if(request_buffer)
			{
				free(request_buffer);
				request_buffer= NULL;
			}

			cout<<"�Ǹ�Ŀ¼ check buffer size error."<<endl;
			return -1;
		}*/
	}

	//use Host
	tmp_size = strlen("HOST: ") + (int)strlen(host) + 3;

	if(CheckBufferSize(&request_buffer , &buffer_size , tmp_size + 128) )//
	{
		if(url)
		{
			free(url);
			url= NULL;
		}

		if(request_buffer)
		{
			free(request_buffer);
			request_buffer= NULL;
		}

		cout<<"Host: check buffer size error."<<endl;
		return -1;
	}
	strcat(request_buffer,"HOST: ");
	strcat(request_buffer,host);
	strcat(request_buffer,"\r\n");

	
	//**********************hide_user_agent��user_agent������
	if(!hide_agent && user_agent == NULL)
	{
		tmp_size = (int)strlen("User-Agent: ") + (int)strlen(DEFAULT_USER_AGENT)
			+ strlen(VERSION) + 4;

		if(CheckBufferSize(&request_buffer , &buffer_size , tmp_size) )//
		{
			if(url)
			{
				free(url);
				url= NULL;
			}

			if(request_buffer)
			{
				free(request_buffer);
				request_buffer= NULL;
			}

			cout<<"User-Agent : check buffer size error."<<endl;
			return -1;
		}
		//strcat(request_buffer,"User-Agent: ");
		//strcat(request_buffer,DEFAULT_USER_AGENT);
		//strcat(request_buffer,VERSION);
		//strcat(request_buffer,"\r\n");
	}
	else if(!hide_agent)
	{
		tmp_size = (int)strlen("User-Agent: ") + (int)strlen(user_agent) + 3;

		if(CheckBufferSize(&request_buffer , &buffer_size , tmp_size) )//
		{
			if(url)
			{
				free(url);
				url= NULL;
			}

			if(request_buffer)
			{
				free(request_buffer);
				request_buffer= NULL;
			}

			cout<<"check buffer size error."<<endl;
			return -1;
		}
		//strcat(request_buffer,"User-Agent: ");
		//strcat(request_buffer,user_agent);
		//strcat(request_buffer,"\r\n");
	}
	
	tmp_size = (int)strlen("Connection: Close\r\n\r\n");

	if(CheckBufferSize(&request_buffer , &buffer_size , tmp_size) )//	
	{
		if(url)
		{
			free(url);
			url= NULL;
		}

		if(request_buffer)
		{
			free(request_buffer);
			request_buffer= NULL;
		}

		cout<<"check buffer size error."<<endl;
		return -1;
	}
	//strcat(request_buffer,"Connection: Keep-Alive\r\n\r\n");
	strcat(request_buffer,"Connection: Close\r\n\r\n");

	
	//���µ���request_buffer���ڴ�ռ䣬�ͷŶ���Ŀռ�
	tmp = (char *)realloc(request_buffer,strlen(request_buffer)+1);
	if(tmp == NULL)
	{
		if(url)
		{
			free(url);
			url = NULL;
		}
		if(request_buffer)
		{
			free(request_buffer);
			request_buffer = NULL;
		}

		cout<<"realloc for the request_buffer error."<<endl;
		return -1;
	}
	

	request_buffer = tmp;
	if(*sock_ptr != -1)
	{
		sock = *sock_ptr;
		cout<<"using the privous sock."<<*sock_ptr<<endl;
	}
	else
	{
		//����һ���µ��׽���
		sock = CreateSocket(host,port);

		if(sock == -1)//invalid host
		{
			if(url)
			{
				free(url);
				url = NULL;
			}
			if(request_buffer)
			{
				free(request_buffer);
				request_buffer = NULL;
			}
			return -3;
		}
		if(sock == -2)//out of ip block
		{
			if(url)
			{
				free(url);
				url = NULL;
			}
			if(request_buffer)
			{
				free(request_buffer);
				request_buffer = NULL;
			}
			return -2;
		}
	}
	
	ret = send(sock,request_buffer,strlen(request_buffer),0);
	if(ret == 0)
	{
		cout<<"request buffer is "<<request_buffer<<endl;
		cout<<"send nothing"<<endl;
		if(url)
		{
			free(url);
			url = NULL;
		}
		if(request_buffer)
		{
			free(request_buffer);
			request_buffer = NULL;
		}
		closesocket(sock);

		*sock_ptr = -1;
		return -1;
	}
	if(ret == SOCKET_ERROR)
	{
		//����ʧ�ܣ���Ҫ���¶���һ��socket
		closesocket(sock);
		*sock_ptr = -1;
		cout<<"close the previous socket , and start a new one"<<endl;

		sock = CreateSocket(host,port);


		if(sock == -1)//invalid host
		{
			if(url)
			{
				free(url);
				url = NULL;
			}
			if(request_buffer)
			{
				free(request_buffer);
				request_buffer = NULL;
			}
			return -3;
		}
		if(sock == -2)//out of ip block
		{
			if(url)
			{
				free(url);
				url = NULL;
			}
			if(request_buffer)
			{
				free(request_buffer);
				request_buffer = NULL;
			}
			return -2;
		}

		if(send(sock,request_buffer,strlen(request_buffer),0) == SOCKET_ERROR)
		{
			if(url)
			{
				free(url);
				url = NULL;
			}
			if(request_buffer)
			{
				free(request_buffer);
				request_buffer = NULL;
			}
			closesocket(sock);

			*sock_ptr = -1;
			cout<<"send error."<<endl;
			return -1;
		}
	}

	if(url)
	{
		free(url);
		url = NULL;
	}
	if(request_buffer)
	{
		free(request_buffer);
		request_buffer = NULL;
	}


	//�洢��ҳͷ��Ϣ
	char header_buffer[HEADER_BUF_SIZE];
	memset(header_buffer,0,HEADER_BUF_SIZE);

	ret = ReadHeader(sock,header_buffer);
	
	if(ret<0)
	{
		closesocket(sock);
		*sock_ptr = -1;
		return -1;
	}
	if(strlen(header_buffer) == 0)
	{
		cout<<"the length of header buffer is 0"<<header_buffer<<endl;
		cout<<"str_url :"<<str_url<<endl<<endl;
		closesocket(sock);
		*sock_ptr = -1;
		return -1;
	}

	//������ҳͷ��Ϣ
	CPage ipage;
	ipage.ParseHeaderInfo(header_buffer);

	if(ipage.status_code == -1)
	{
		closesocket(sock);
		*sock_ptr = -1;
		cout<<"header buffer :"<<header_buffer<<endl;
		cout<<"header error : not find the http"<<endl;
		return -1;
	}

	//HTTP״̬�������ж�
	if(ipage.status_code == 301 || ipage.status_code == 302)
	{
		if(ipage.location.empty() || ipage.location.size()>URL_LEN)
		{
			closesocket(sock);
			*sock_ptr = -1;
			cout<<"error location"<<endl;
			return -1;
		}
		else
		{
			char *loc = strdup(ipage.location.c_str());
			*location = loc;
			closesocket(sock);
			*sock_ptr = -1;
			return -300;//��ʾ�Ѿ��ض���

		}
	}

	if(ipage.status_code<200 || ipage.status_code >299)
	{
		closesocket(sock);
		*sock_ptr = -1;
		cout<<"the HTTP Status Code is"<<ipage.status_code<<endl;
		return -1;
	}

	//�����ҳ����ΪͼƬ������ȡͼƬ
	if(ipage.content_type.find("image") != string::npos)
	{
		//closesocket(sock);
		//*sock_ptr = -1;
		cout<<"Get the image content type"<<endl;

		return -4;
	}
	if(ipage.header_content_length == -1)
	{
		closesocket(sock);
		*sock_ptr = -1;
		cout<<"error content-length"<<endl;
		return -1;
	}
	if(ipage.header_content_length == 0 || ipage.header_content_length<20)
	{
		ipage.header_content_length = DEFAULT_PAGE_BUF_SIZE;
	}

	if(ipage.header_content_length > MAX_PAGE_BUF_SIZE)
	{
		cout<<"�����ҳ�ĳ��ȴ���10M��������˵���"<<endl;
		closesocket(sock);
		*sock_ptr = -1;
		return -1;
	}

	page_buffer = (char *)malloc(ipage.header_content_length);

	if(page_buffer == NULL)
	{
		closesocket(sock);
		*sock_ptr = -1;
		cout<<"can not malloc for the page buffer"<<endl;
		return -1;
	}
	
	//��ʼ��ȡ��ҳ����Ϣ
	fd_set rfds;
	struct timeval tv;

	//��socket�׽����ļ����������óɷ�������ʽ
	int nRet;
	u_long l0n0ff =true;
	nRet = ioctlsocket(sock,FIONBIO,(u_long FAR *)&l0n0ff);

	if(nRet == SOCKET_ERROR)
	{
		closesocket(sock);
		*sock_ptr = -1;
		if(page_buffer)
		{
			free(page_buffer);
			page_buffer = NULL;
		}
		cout<<"No blocking error"<<endl;
		return -1;
	}

	//��һ��whileѭ����ȡ��ҳ����Ϣ
	int pre_ret = 0;

	while(ret>0)
	{
		FD_ZERO(&rfds);//����rfds���ļ�����������
		FD_SET(sock,&rfds);//��sock���뵽rfds��
		if(bytes_read == ipage.header_content_length)
		{
			tv.tv_sec = 1;
		}
		else
			tv.tv_sec = timeout;

		tv.tv_usec = 0;

		if(DEFAULT_TIMEOUT >= 0)
		{
			select_ret = select(sock+1,&rfds,NULL,NULL,&tv);
		}
		else//no timeout
			select_ret = select(sock+1,&rfds,NULL,NULL,NULL);

		if(select_ret == 0 &&timeout < 0)//��ʱ
		{
			closesocket(sock);
			*sock_ptr = -1;
			if(page_buffer)
			{
				free(page_buffer);
				page_buffer = NULL;
			}
			cout<<"Time out"<<endl;
			return -1;
		}
		else if(select_ret == SOCKET_ERROR)
		{
			closesocket(sock);
			*sock_ptr = -1;
			if(page_buffer)
			{
				free(page_buffer);
				page_buffer = NULL;
			}
			cout<<"select error"<<endl;
			return -1;
		}

		ret = recv(sock,page_buffer+bytes_read,ipage.header_content_length,0);
		//char *s=(char *)malloc(ret);
		//memcpy(s,page_buffer,ret);
		cout<<ret<<endl;
		//ret = recv(sock,page_buffer,ipage.header_content_length,0);
		if(ret == 0)
			break;
		if(ret == SOCKET_ERROR && pre_ret == 0)
		{
			closesocket(sock);
			*sock_ptr = -1;
			if(page_buffer)
			{
				free(page_buffer);
				page_buffer = NULL;
			}
			cout<<"receive error"<<endl;
			return -1;
		}
		else if(ret == SOCKET_ERROR && pre_ret)
			break;

		pre_ret =ret;
		bytes_read+=ret;

		//��ֹ������ҳ��Ϣ�й�����ҳ���С����Ϣ����ȷ
		//������ٶ����ռ��page buffer
		if(ret>0)
		{
			page_buffer = (char *)realloc(page_buffer,bytes_read+ipage.header_content_length);

			if(page_buffer == NULL)
			{
				closesocket(sock);
				*sock_ptr = -1;
				if(page_buffer)
				{
					free(page_buffer);
					page_buffer = NULL;
				}
				cout<<"page buffer realloc error"<<endl;
				return -1;
			}
		}
	}

	//���ػ�����������С������
	page_buffer = (char *)realloc(page_buffer,bytes_read+1);

	if(page_buffer == NULL)
	{
		closesocket(sock);
		*sock_ptr = -1;
		if(page_buffer)
		{
			free(page_buffer);
			page_buffer = NULL;
		}
		cout<<"page buffer realloc error"<<endl;
		return -1;
	}

	page_buffer[bytes_read] = '\0';

	if(file_buf == NULL)
	{
		if(page_buffer)
		{
			free(page_buffer);
			page_buffer = NULL;
		}
	}
	else
	{
		char *tmp;

		tmp = (char *)malloc(strlen(header_buffer)+1);

		if(tmp == NULL)
		{
			closesocket(sock);
			*sock_ptr = -1;
			if(page_buffer)
			{
				free(page_buffer);
				page_buffer = NULL;
			}
			cout<<"file buffer malloc error"<<endl;
			return -1;
		}

		strncpy(tmp,header_buffer,strlen(header_buffer)+1);

		*file_head_buf = tmp;
		*file_buf = page_buffer;

	}
	
	*sock_ptr = sock;
	return bytes_read;

}



/*************************************************
**function:�����׽����ļ������������ҵ���NoBlockingConnect()
		   ��Ŀ���������������
**success :����sock[�׽����ļ�������]
**fail    :����-1  [��������]
           ����-2  [��IP������Χ��]
*************************************************/
int CHttp::CreateSocket(const char *host,int port)
{
	int sockfd;//�ļ�������
	struct sockaddr_in address;// socket address
	struct hostent *hp;

	unsigned long inaddr;
	int ret;

	CUrl iurl;
	char * ip = iurl.GetIpByHost(host);//ͨ�������õ�ip��ַ

	if(ip == NULL)
	{
		return -1;
	}
	else
	{
		if(iurl.IsValidIp(ip))//��ip������Χ��
		{
			inaddr = (unsigned long)inet_addr(ip);//���ַ���ipת��Ϊ32λ�������ֽ���

			if(inaddr == INADDR_NONE)
			{
				delete []ip;
				ip = NULL;
				cout<<"invalid ip"<<endl;
				return -1;
			}

			memcpy((char *)&address.sin_addr,&inaddr,sizeof(inaddr));

			delete []ip;
			ip = NULL;
		}
		else//��ip������Χ��
		{
			delete []ip;
			ip = NULL;
			return -2;
		}
	}

	//����NoBlockingConnect����Ҫʹ�õĵ�ַ
	hp = gethostbyname(host);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
	address.sin_port   = htons(port);

	//�����׽���
	sockfd = -1;
	sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if(sockfd == INVALID_SOCKET)
	{
		cout<<"create socket error"<<endl;
		return -1;
	}

	int optval = 1;
	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval) == SOCKET_ERROR))
	{
		cout<<"set socket option in CreateSocket error"<<endl;
		return -1;
	}

	//��������
	ret = NoBlockingConnect(sockfd,(struct sockaddr *)&address,DEFAULT_TIMEOUT);
	//ret = connect(sockfd,(SOCKADDR *)&address,sizeof(address));

	if(ret == SOCKET_ERROR)
	{
		closesocket(sockfd);
		cout<<"can not connect"<<endl;
		return -1;
	}
	if(ret == 0)
	{
		cout<<"the web has been connected success"<<endl;
		return sockfd;
	}
	

	
}


/*******************************************
**function:ͨ��IO���õķ�����ȡ��ҳͷ��Ϣ
**success :����bytes_read[��ҳͷ��Ϣ����ʵ����]
**fail    :����-1
*******************************************/
int CHttp::ReadHeader(int sock, char *header_ptr)
{
	fd_set rfds;//���ļ��������
	struct timeval tv;
	int bytes_read =0,new_lines = 0,ret,select_ret;

	int nRet;
	u_long l0n0ff =true;
	nRet = ioctlsocket(sock,FIONBIO,(u_long FAR *)&l0n0ff);

	if(nRet == SOCKET_ERROR)
	{
		cout<<"No blocking error in ReadHeader"<<endl;
		return -1;
	}

	//��һ��whileѭ������ȡ��ҳͷ��Ϣ
	while(new_lines !=2 && bytes_read != HEADER_BUF_SIZE-1)
	{
		
		FD_ZERO(&rfds);//������ļ�����������
		FD_SET(sock,&rfds);//���׽����ļ��������ӵ��ļ�������������
		tv.tv_sec = timeout;//������ȴ�ʱ��
		tv.tv_usec = 0;

		if(timeout>0)
			select_ret = select(sock+1,&rfds,NULL,NULL,&tv);
		else
			select_ret = select(sock+1,&rfds,NULL,NULL,NULL);

		if(select_ret == 0 && timeout<0)
		{
			cout<<"select == 0 && timeout < 0"<<endl;
			return -1;
		}
		else if(select_ret == SOCKET_ERROR )
		{
			 cout<<"select error"<<endl;
			 return -1;
		}
		ret = recv(sock,header_ptr,1,0);
		if(ret == SOCKET_ERROR)
		{
			cout<<"receive error in ReadHeader"<<endl;
			return -1;
		}

		bytes_read++;
		if(*header_ptr == '\r')
		{
			header_ptr++;
			continue;
		}
		else if(*header_ptr == '\n')
			new_lines++;
		else
			new_lines = 0;
		header_ptr++;
	}

	header_ptr -=2;
	*header_ptr = '\0';
	return bytes_read;

}


/****************************************
**function: ��CreateSocket�������ã�ͨ��
            IO���÷�ʽ���ӷ�����
**success ������0��sockfd
**fail    : ����-1

**sockfd  : �׽����ļ�������
**address : �����׽��ֵ�ַ�ṹ
**sec     : ��ȴ�ʱ��
****************************************/
int CHttp::NoBlockingConnect(int sockfd,struct sockaddr* address,int sec)
{
	int nRet;
	int status;
	fd_set mask;//д�ļ�����������
	struct timeval timeout;

	//�����׽���Ϊ�������ṹ
	u_long l0n0ff =true;
	nRet = ioctlsocket(sockfd,FIONBIO,(u_long FAR *)&l0n0ff);

	if(nRet == SOCKET_ERROR)
	{
		cout<<"No blocking error in NoBlockingConnect"<<endl;
		return -1;
	}

	if(connect(sockfd,address,sizeof(sockaddr))== 0)//���ӳɹ�
	{
		cout<<"connect the NoBlockingConnect success"<<endl;
		//���±�ɷ�������ʽ
		l0n0ff =false;
		nRet = ioctlsocket(sockfd,FIONBIO,(u_long FAR *)&l0n0ff);
		return sockfd;
	}

	FD_ZERO(&mask);//����д�ļ�������
	FD_SET(sockfd,&mask);//���׽����ļ�������д�뵽д�ļ�����������
	timeout.tv_sec = sec;
	timeout.tv_usec= 0;

	status=select(sockfd+1,NULL,&mask,NULL,&timeout);//IO����

	switch(status)
	{
	case SOCKET_ERROR:
		l0n0ff =false;
		nRet = ioctlsocket(sockfd,FIONBIO,(u_long FAR *)&l0n0ff);
		cout<<"select in NoBlockingConnect error"<<endl;
		return -1;

	case 0:
		l0n0ff =false;
		nRet = ioctlsocket(sockfd,FIONBIO,(u_long FAR *)&l0n0ff);
		cout<<"select in NoBlockingConnect timeout"<<endl;
		return -1;

	default:
		FD_CLR(sockfd,&mask);
		l0n0ff =false;
		nRet = ioctlsocket(sockfd,FIONBIO,(u_long FAR *)&l0n0ff);
		cout<<"connect success"<<endl;
		return 0;
	}

}


/*****************************************
**function: ���*buf��ָ���ڴ�ռ�ʣ��ֵ��
            �����more,�����ټ�more+1��λ��
			�ڴ�ռ�
**success : ���� 0
**fail    : ���� -1
*****************************************/
int CHttp::CheckBufferSize(char **buf, int *buf_size, int more)
{
	char *tmp;

	int room_left = *buf_size - (strlen(*buf)+1);

	if(room_left > more)
		tmp = (char *)realloc(*buf, *buf_size + more + 1);//ʣ��ֵ����more,���ʱ������Ҫ�����ڴ�ռ�ĳ��ȣ����ȼ�more+1
	if(tmp == NULL)
		return -1;

	*buf = tmp;
	*buf_size += more+1;
	return 0;
}