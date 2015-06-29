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
	RefLinkForPSE page_link[MAX_URL_REFERENCES];//����URL��Ϣ<-->URL��������Ϣ[����URLָ����Ϊ����׼��������] ��ÿ����ҳ����ܱ���1000������
	int           page_link_number;////��������ĳ���

	//link for history 
	RefLinkForHistory history_page_link[MAX_URL_REFERENCES/2];//����URL��Ϣ[���URLָ����Ϊ��ʷ��ҳ�浵׼��������]
	int               history_page_link_number;

	//map and vector
	map<string,string> map_link_for_pse;//����URL��Ϣ<-->URL��������Ϣ[����URLָ����Ϊ����׼��������]
	vector<string> vector_link_for_history;

	//page type
	enum page_type my_page_type;//��ҳ������

public:
	CPage();
	CPage(string str_url , string str_location , char *header , char *body , int body_length);
	~CPage();

	//parse header information from the header content
	void ParseHeaderInfo(string header);////������ҳͷ��Ϣ

	//parse hyperlinks from the page content
	bool ParseHyperLinks();//����ҳ����ȡ����������Ϣ

	bool NormalizeUrl(string& str_url);//�ж�strUrl�ǲ��������url

	bool IsFilterLink(string plink);//�ж�plink�����ǲ���Ҫ���˵�

private:
	//parse header information from the header content
	 void GetStatusCode(string header);//�õ�״̬��
	 void GetContentLength(string header);//����ҳͷ��Ϣ����ȡ����ҳ��ĳ���
	 void GetConnectionState(string header);//�õ�����״̬
	 void GetLocation(string header);//�õ��ض�����Ϣ
	 void GetCharset(string header);//�õ��ַ���
	 void GetContentEncoding(string header);//�õ���ҳ�����
	 void GetContentType(string header);//�õ���ҳ������
	 void GetTransferEncoding(string header);//�õ���ҳ��Ĵ�����뷽ʽ

	 //parse hyperlink from the web 
	 bool GetContentLinkInfo();//����ҳ������ȡ��������������Ϣ�ı�ʶ
	 bool GetLinkInfoForPSE();
	 bool GetLinkInfoForHistory();
	 bool FindRefLinkForPSE();//�õ�Ϊ��������׼���ĳ�����
	 bool FindRefLinkForHistory();//�õ�Ϊ��ʷ��ҳ�浵׼���ĳ�����

};

#endif 