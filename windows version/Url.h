#ifndef _URL_H
#define _URL_H
#include <string>

const unsigned int URL_LEN = 256;
const unsigned int HOST_LEN = 256;

using namespace std;


enum url_scheme
{
	SCHEME_HTTP,
	SCHEME_FTP,
	SCHEME_INVALID
};

const int DEFAULT_HTTP_PORT = 80;
const int DEFAULT_FTP_PORT = 21;

class CUrl
{
public:
	
	string origin_url;
	enum url_scheme scheme_name;     

	string host_name;
	int port_number;
	string request_path;

public:
	CUrl();
	~CUrl();

	//break an url into scheme, host, port and request
	//result as number variants
	bool ParseUrl(string str_url);

	//break an url into scheme, host, port and request
	//result as argvs
	void ParseUrl(const char *url, char *protocol, int protocol_length,
			char *host, int host_length,
			char *request, int request_length, int *port);

	//get ip address by host name
	char *GetIpByHost(const char *host);

	//judge the infomation of url
	bool IsValidHost(const char *ip);
	bool IsForeignHost(string host);
	bool IsImageUrl(string url);
	bool IsValidIp(const char *ip);
	bool IsVisitedUrl(const char* url); 
	bool IsUnreachedUrl(const char* url);
	bool IsValidHostChar(char ch);

private:
	void ParseScheme(const char * url);


};

#endif 