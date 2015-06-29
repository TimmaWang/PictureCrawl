#ifndef _FILENAME_H
#define _FILENAME_H

#include <string>
using namespace std;

const string DATA_FILE_NAME("WebData.txt");
const string INDEX_FILE_NAME("WebDataIndex.txt");

const string DATA_PSE_FILE("web_infomation_pse");//存放网页体信息
const string DATA_LINK_FOR_PSE_FILE("link_for_pse");//创建一个网页结构库

const string VISITED_FILE("visited_url_pse.txt");//存放访问过的网页url
const string UNVISITED_FILE("unvisited_url_pse.txt");//存放没有访问过的网页url
const string UNREACH_HOST_FILE("unreach_host_pse.txt");//存放不可达的host
const string LINK_FOR_PSE_FILE("link_url_pse.txt");//存放链接的url信息
const string LINK_FOR_HISTORY_FILE("imag_url_for_pse.txt");//存放图片链接信息
const string URL_MD5_FILE("url_md5_pse.txt");
const string PAGE_MD5_FILE("page_md5_pse.txt");
const string IP_BLOCK_FILE("ip_block_pse.txt");

const unsigned int NUMBER_WORKERS = 5;
const unsigned int NUMBER_WORKERS_ON_A_SITE = 4;


//****************************************************


#endif