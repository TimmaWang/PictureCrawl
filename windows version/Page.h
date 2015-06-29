#ifndef _PAGE_H
#define _PAGE_H

#include <list>
#include <map>
#include <vector>
#include <string>

#include "Url.h"

using namespace std;

const int ANCHOR_TEXT_LEN    = 256;
const int MAX_URL_REFERENCES = 1000;
const int URL_REFERENCES_LEN = (URL_LEN+ANCHOR_TEXT_LEN)*MAX_URL_REFERENCES*1/2;
const int MAX_TAG_NUMBERS    = 10000;

//plain text or other
enum page_type
{
	PLAIN_TEXT,
	OTHER
};

//<href src...>, <area src...>
struct RefLinkForPSE
{
	char *link;
	char *anchor_text;
	string str_char_set;
};

struct RefLinkForHistory
{
	char *link;
	
};

class CPage
{
public :

	string url;

	//the information of page header
	string header;
	int    header_length;
	int    status_code;
	int    header_content_length;
	string location;
	bool   connection_state;
	string content_encoding;
	string content_type;
	string char_set;
	string transfer_encoding;

	//the information of page body
	string body_content;
	int    body_content_length;
	string content_no_tags;
	string content_link_info;

	//the link preparing for the search engine
	string link_for_search;
	int    link_for_search_length;

	//the link preparing for the history
	string link_for_history;
	int    link_for_history_length;

	//link for PSE , in a good state
	RefLinkForPSE page_link[MAX_URL_REFERENCES];//保存URL信息<-->URL的描述信息[这里URL指的是为搜索准备的链接] 即每个网页最多能保存1000个链接
	int           page_link_number;////上面数组的长度

	//link for history 
	RefLinkForHistory history_page_link[MAX_URL_REFERENCES/2];//保存URL信息[这个URL指的是为历史网页存档准备的链接]
	int               history_page_link_number;

	//map and vector
	map<string,string> map_link_for_pse;//保存URL信息<-->URL的描述信息[这里URL指的是为搜索准备的链接]
	vector<string> vector_link_for_history;

	//page type
	enum page_type my_page_type;//网页的类型

public:
	CPage();
	CPage(string str_url , string str_location , char *header , char *body , int body_length);
	~CPage();

	//parse header information from the header content
	void ParseHeaderInfo(string header);////解析网页头信息

	//parse hyperlinks from the page content
	bool ParseHyperLinks();//从网页中提取出超链接信息

	bool NormalizeUrl(string& str_url);//判断strUrl是不是正规的url

	bool IsFilterLink(string plink);//判断plink链接是不是要过滤掉

private:
	//parse header information from the header content
	 void GetStatusCode(string header);//得到状态码
	 void GetContentLength(string header);//从网页头信息中提取的网页体的长度
	 void GetConnectionState(string header);//得到连接状态
	 void GetLocation(string header);//得到重定向信息
	 void GetCharset(string header);//得到字符集
	 void GetContentEncoding(string header);//得到网页体编码
	 void GetContentType(string header);//得到网页体类型
	 void GetTransferEncoding(string header);//得到网页体的传输编码方式

	 //parse hyperlink from the web 
	 bool GetContentLinkInfo();//从网页体中提取出包含超链接信息的标识
	 bool GetLinkInfoForPSE();
	 bool GetLinkInfoForHistory();
	 bool FindRefLinkForPSE();//得到为搜索引擎准备的超链接
	 bool FindRefLinkForHistory();//得到为历史网页存档准备的超链接

};

#endif 