#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <string>
#include <WinSock2.h>
#include <string.h>

#include "Url.h"
#include "MD5.h"
#include "StrFunction.h"

//judge x == "." ?
#define DOTP(x) ((*(x) == '.') && (!*(x + 1)))
//judge x == ".." ?
#define DDOTP(x) ((*(x) == '.') && (*(x + 1) == '.') && (!*(x + 2)))

map<string,string> map_cache_host_lookup;
extern vector<string> vector_unreach_host;
extern set<string> set_visited_url_md5;
extern map<unsigned long,unsigned long> map_ip_block;
typedef map<string,string>::value_type value_type_chl;


struct scheme_data
{
	char *leading_string;
	int default_port;
	int enabled;
};


//supported schemes:
static scheme_data supported_schemes[]=
{
	{ "http://", DEFAULT_HTTP_PORT, 1 },
	{ "ftp://" , DEFAULT_FTP_PORT , 1 },
	{  NULL    , -1               , 0 }
};


//return the scheme type if the scheme is suported
//or return the scheme invalid
void CUrl::ParseScheme(const char * url)
{
	for(int i=0;supported_schemes[i].leading_string;i++)
	{
		if(0 == strncmp(url,supported_schemes[i].leading_string,
			strlen(supported_schemes[i].leading_string)))//可能出错，在调用函数strcmp时。
		{
			if(supported_schemes[i].enabled)
			{
				this->scheme_name = (enum url_scheme) i;
				return ;
			}
			else
			{
				this->scheme_name = SCHEME_INVALID;
				return ;
			}
		}
	}

	this->scheme_name = SCHEME_INVALID;
	return ;
}


/************************************************************************
 *  Function name: ParseUrl
 *  Input argv:
 *  	-- strUrl: url
 *  Output argv:
 *  	--
 *  Return:
   	true: success
   	false: fail
 *  Fucntion Description: break an URL into scheme, host, port and request.
 *  			result as member variants
************************************************************************/
bool CUrl::ParseUrl(string str_url)
{
	char protocol[10];
	char host[HOST_LEN];
	char request[256];
	int  port = -1;

	memset(protocol,0,sizeof(protocol));
	memset(host,0,sizeof(host));
	memset(request,0,sizeof(request));

	this->ParseScheme(str_url.c_str());
	if(this->scheme_name != SCHEME_HTTP)
	{
		return false;
	}

	ParseUrl(str_url.c_str(),protocol,sizeof(protocol),
		host,sizeof(host),request,sizeof(request),
		&port);

	this->origin_url = str_url;
	this->host_name = host;
	this->request_path = request;

	if(port>0)
	{
		this->port_number=port;
	}

	return true;

}


/************************************************************************
 *  Function name: ParseUrl
 *  Input argv:
 *  	-- url: host name
 *  	-- protocol: result protocol
 *  	-- protocol_length: protocol length
 *  	-- host: result host
 *  	-- host_length: host length
 *  	-- request: result request
 *  	-- request_length: request length
 *  Output argv:
 *  	--
 *  Return:
   	true: success
   	false: fail
 *  Fucntion Description: break an URL into scheme, host, port and request.
 *  			result as argvs
************************************************************************/
void CUrl::ParseUrl( const char *url, char *protocol, int protocol_length,
			char *host, int host_length,
			char *request, int request_length, int *port)
{
	char *work,*ptr,*ptr2;

	*protocol = *host = *request = 0;
	*port = 80;

	int len = strlen(url);
	work = new char[len + 1];
	memset(work, 0, len+1);
	strncpy(work, url, len);

	//find the protocol ,if there is
	ptr = strchr(work,':');
	if( ptr != NULL )
	{
		*(ptr++) = 0;
		strncpy( protocol, work, protocol_length );
	}
	else 
	{
		strncpy( protocol, "HTTP", protocol_length );
		ptr = work;
	}

	//skip the mark "//"
	if( (*ptr=='/') && (*(ptr+1)=='/') )
		ptr+=2;

	//find the host , if there is
	ptr2 = ptr;
	while( IsValidHostChar(*ptr2) && *ptr2 )
		ptr2++;
	*ptr2 = 0;
	strncpy( host, ptr, host_length );

	// find the request , if there is
	int offset = ptr2 - work;
	const char *pStr = url + offset;
	strncpy( request, pStr, request_length );

	// find the port number , if there is
	ptr = strchr( host, ':' );
	if( ptr != NULL )
	{
		*ptr = 0;
		*port = atoi(ptr+1);
	}

	delete [] work;
	work = NULL;
}

CUrl::CUrl()
{
	this->origin_url = "";
	this->scheme_name = SCHEME_INVALID;
	this->host_name = "";
	this->port_number = DEFAULT_HTTP_PORT;
	this->request_path = "";
}


/****************************************************************************
 *  Function name: GetIpByHost
 *  Input argv:
 *  	-- host: host name
 *  Output argv:
 *  	--
 *  Return:
   	ip: sucess
   	NULL: fail
 *  Function Description: get the ip address by host name
****************************************************************************/
char * CUrl::GetIpByHost(const char *host)
{
	//null pointer
	if(!host)
	{
		return NULL;
	}

	//invalid host
	if(!IsValidHost(host))
	{
		return NULL;
	}

	unsigned long inaddr = 0;
	char *result = NULL;
	int len = 0;


	inaddr = (unsigned long)inet_addr( host );
	
	// host is just ip
	if ( inaddr != INADDR_NONE)
	{ 
		len = strlen(host);
		//pthread_mutex_lock(&mutexMemory);
		result = new char[len+1];
		//pthread_mutex_unlock(&mutexMemory);
		memset(result, 0, len+1);
		memcpy(result, host, len);

		return result;

     } 
	else 
	{
		map<string,string>::iterator it  = map_cache_host_lookup.find(host);

		// find in host lookup cache
		if( it != map_cache_host_lookup.end() )
		{	
			const char * strHostIp;

			strHostIp = (*it).second.c_str();
			inaddr = (unsigned long)inet_addr( strHostIp );

			if ( inaddr != INADDR_NONE )
			{ 
				len = strlen(strHostIp);
				//pthread_mutex_lock(&mutexMemory);
				result = new char[len+1];
				//pthread_mutex_unlock(&mutexMemory);
				memset( result, 0, len+1 );
				memcpy( result, strHostIp, len );
				
				return result;
        	}
		}
	}

	//通过上面的方法我们都没有查找，这个时候我们只能通过DNS server查找了
	struct hostent *hp;

	hp = gethostbyname(host);
	//通过主机号或者说是域名得到hostent结构，这个结构包含主机号或者说域名的很多信息
	if(hp == NULL)
	{
		return NULL;
	}

	struct in_addr in;
	in.S_un.S_addr = (* (unsigned long *)hp->h_addr_list[0]);

	char  *buf = NULL;
	buf = inet_ntoa(in);
	
	if(buf == NULL)
	{
		cout<<"host change to ip error"<<endl;
		return NULL;
	}
	else
	{
		if(map_cache_host_lookup.find(host) == map_cache_host_lookup.end())
			map_cache_host_lookup.insert(value_type_chl (host ,buf));

	}

	len = strlen(buf);
	result = new char[len + 1];

	memset( result, 0, len+1 );
	memcpy( result, buf, len );

	
	return result;
	
	


}


/**********************************************************************************
 *  Function name: IsValidHostChar
 *  Input argv:
 *  	-- ch: the character for testing
 *  Output argv:
 *  	-- 
 *  Return:
   	true: is valid
   	false: is invalid
 *  Function Description: test the specified character valid
 *  			for a host name, i.e. A-Z or 0-9 or -.:
**********************************************************************************/
bool CUrl::IsValidHostChar(char ch)
{
	return ( isalpha(ch) || isdigit(ch) ||
		ch=='-' || ch=='.' ||ch=='_' ||ch==':');
}


/**********************************************************************************
 *  Function name: IsValidHost
 *  Input argv:
 *  	-- ch: the character for testing
 *  Output argv:
 *  	-- 
 *  Return:
   	true: is valid
   	false: is invalid
 *  Function Description: test the specified character valid
 *  			for a host name, i.e. A-Z or 0-9 or -.:
**********************************************************************************/
bool CUrl::IsValidHost(const char *host)
{
	if(!host)
	{
		return false;
	}

	//in case host like "www", "pku" and so on
	if(strlen(host) < 6)
	{
		return false;
	}

	char ch;
	for(int i=0;i<strlen(host);i++)
	{
		ch = host[i];
		if(!IsValidHostChar(ch))
		{
			return false;
		}
	}

	return true;
}


/**********************************************************************************
 *  Function name: IsVisitedUrl
 *  Input argv:
 *  	-- url: url
 *  Output argv:
 *  	-- 
 *  Return:
   	true: is visited
   	false: not visited
 *  Function Description: test the url visited by the MD5
**********************************************************************************/
bool CUrl::IsVisitedUrl(const char *url)
{
	if(!url)
	{
		return false;
	}

	CMD5 md5;

	//turn the string of url into the MD5 digest
	md5.GenerateMd5((unsigned char *)url,strlen(url));

	string str_digest=md5.ToString();

	//check the set , find whether the digest is exist
	if(set_visited_url_md5.find(str_digest) != set_visited_url_md5.end())
	{
		return true;
	}
	else
	{
		return false;
	}

}


/**********************************************************************************
 *  Function name: IsValidIp
 *  Input argv:
 *  	-- ip: ip
 *  Output argv:
 *  	-- 
 *  Return:
   	true: inside the ip block
   	false: outside the ip block
 *  Function Description: decide teh ip whether or not inside the ip block
**********************************************************************************/
bool CUrl::IsValidIp(const char *ip)
{
	if(ip == NULL)
	{
		return false;
	}

	unsigned long inaddr = (unsigned long)inet_addr(ip);
	//invalid ip
	if(inaddr ==INADDR_NONE)
	{
		return false;
	}

	if(map_ip_block.size()>0)
	{
		map<unsigned long,unsigned long>::iterator it;
		for(it=map_ip_block.begin();it!=map_ip_block.end();it++)
		{
			unsigned long ret;
			ret = inaddr &~ ((*it).second);
			if(ret == (*it).first)
			{
				return true;
			}
		}
		return false;
	}
	// if block range is not given, we think it inside also
	return true;
}

/*
 * If it is, return true; otherwise false
 * not very precise
 */
bool CUrl::IsForeignHost(string host)
{
	if( host.empty() ) return true;
	if( host.size() > HOST_LEN ) return true;

	unsigned long inaddr = 0;

	inaddr = (unsigned long)inet_addr( host.c_str() );
	if ( inaddr != INADDR_NONE){ // host is just ip
		return false;
	}

	string::size_type idx = host.rfind('.');
	string tmp;
	if( idx != string::npos ){
		tmp = host.substr(idx+1);
	}

	CStrFunction::StrToLower( tmp, tmp.size() );
	const char *home_host[] ={
		"cn","com","net","org","info",
		"biz","tv","cc", "hk", "tw"
	};

	int home_host_num = 10;

	for(int i=0; i<home_host_num; i++){
		if( tmp == home_host[i] )
			return false;
	}

	return true;
}
	
	
bool CUrl::IsImageUrl(string url)
{
	if( url.empty() ) return false;
	if( url.size() > HOST_LEN ) return false;

	string::size_type idx = url.rfind('.');
	string tmp;
	if( idx != string::npos ){
		tmp = url.substr(idx+1);
	}

	CStrFunction::StrToLower( tmp, tmp.size() );
	const char *image_type[] ={
		"gif","jpg","jpeg","png","bmp",
		"tif","psd"
	};

	int image_type_num = 7;

	for (int i=0; i<image_type_num; i++)
	{
		if( tmp == image_type[i] )
			return true;
	}

	return false;
}

CUrl::~CUrl()
{

}