#ifndef _CRAWL_H
#define _CRAWL_H

#include "FileName.h"
#include "Http.h"
#include "StrFunction.h"
#include "Url.h"
#include "Page.h"
#include "PSEFile.h"
#include "ISAMFile.h"
#include "LinkForPSEFile.h"

using namespace std;

class CCrawl
{
public:
	string input_file_name;//种子url的文件名称
	string output_file_name;//保存我们已经访问过的文件名称

	CISAMFile isam_file;//ISAM file handle

	ofstream ofs_visited_url_file;//visited.all的文件句柄
	ofstream ofs_link_for_pse_file;//LinkForPSE.url的文件句柄
	ofstream ofs_link_for_history_file;//LinkForHistory.url的文件句柄
	ofstream ofs_unreached_host_file;//UnreachHost.list的文件句柄
	ofstream ofs_visited_url_md5_file;//PSEMd5.visitedurl的文件句柄
	ofstream ofs_visited_page_md5_file;//PSEMd5.visitedpage的文件句柄
	ofstream ofs_unreached_url_file;//unreached url file handle

public:
	CCrawl();
	CCrawl(string input_file_str,string output_file_str);
	~CCrawl();

	//CCrawl类中最重要的函数
	void DoCrawl();

	//根据url以及套接字文件描述符抓取url对应的网页
	void DownroadFile(CPSEFile *pse_file_ptr,CLinkForPSEFile *link_for_pse_file_ptr,
		CUrl iurl,int nGsock);

	//每个线程函数start都调用的函数
	void Fetch(void *arg);

	//如果url满足条件，加入到mapurl[等待访问的url]容器中去
	void AddUrl(const char * url);

	//得到已经访问过的URL对应的MD5值，放入set_visited_url_md5中
	void GetVisitedUrlMd5();

	//得到已经访问过的web网页体对应的MD5值，放入set_visited_page_md5中
	void GetVisitedPageMd5();

	//得到阻塞的IP，放入map_ip_block容器中
	void GetIpBlock();

	//得到不可到达的主机号,放入set_unreach_host_md5中
	void GetUnreachHostMd5();

	//打开所有的输出流
	void OpenFilesForOutput();

	//save in the process

	//将抓取的网页以PSE格式存储
	void SavePseRawData(CPSEFile *pse_file_ptr,CUrl *url_ptr,CPage *page_ptr);

	//从抓取的网页中提取超链接信息，建立网页结构库
	void SaveLinkForPseRawData(CLinkForPSEFile *link_for_pse_file_ptr,CUrl *url_ptr,CPage *page_ptr);

	//将抓取的网页?????????????????????????????????????????????????????????????????????????????????????????????????????????????
	void SaveIsamRawData(CUrl *url_ptr,CPage *page_ptr);

	//保存已经访问过的url
	void SaveVisitedUrl(string url);

	//保存不可到达的主机号
	void SaveUnreachHost(string host);

	//保存为搜索引擎准备的超链信息
	void SaveLinkForPSE(CPage *page_ptr);

	//???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
	bool SaveLink4SE031121(void *arg);

	//保存为历史网页存档准备的超链接信息
	void SaveLinkForHistory(CPage * page_ptr);

	//save while the process running
	
	//保存已经访问过的Url对应的md5值
	void SaveVisitedUrlMd5(string md5);

	//保存已经访问过的page对应的md5值
	void SaveVisitedPageMd5(string md5);

};


#endif