#ifndef _FILENAME_H
#define _FILENAME_H

#include <string>
using namespace std;

const string DATA_FILE_NAME("WebData.txt");
const string INDEX_FILE_NAME("WebDataIndex.txt");

const string DATA_PSE_FILE("web_infomation_pse");//�����ҳ����Ϣ
const string DATA_LINK_FOR_PSE_FILE("link_for_pse");//����һ����ҳ�ṹ��

const string VISITED_FILE("visited_url_pse.txt");//��ŷ��ʹ�����ҳurl
const string UNVISITED_FILE("unvisited_url_pse.txt");//���û�з��ʹ�����ҳurl
const string UNREACH_HOST_FILE("unreach_host_pse.txt");//��Ų��ɴ��host
const string LINK_FOR_PSE_FILE("link_url_pse.txt");//������ӵ�url��Ϣ
const string LINK_FOR_HISTORY_FILE("imag_url_for_pse.txt");//���ͼƬ������Ϣ
const string URL_MD5_FILE("url_md5_pse.txt");
const string PAGE_MD5_FILE("page_md5_pse.txt");
const string IP_BLOCK_FILE("ip_block_pse.txt");

const unsigned int NUMBER_WORKERS = 5;
const unsigned int NUMBER_WORKERS_ON_A_SITE = 4;


//****************************************************


#endif