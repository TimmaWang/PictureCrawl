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
	string input_file_name;//����url���ļ�����
	string output_file_name;//���������Ѿ����ʹ����ļ�����

	CISAMFile isam_file;//ISAM file handle

	ofstream ofs_visited_url_file;//visited.all���ļ����
	ofstream ofs_link_for_pse_file;//LinkForPSE.url���ļ����
	ofstream ofs_link_for_history_file;//LinkForHistory.url���ļ����
	ofstream ofs_unreached_host_file;//UnreachHost.list���ļ����
	ofstream ofs_visited_url_md5_file;//PSEMd5.visitedurl���ļ����
	ofstream ofs_visited_page_md5_file;//PSEMd5.visitedpage���ļ����
	ofstream ofs_unreached_url_file;//unreached url file handle

public:
	CCrawl();
	CCrawl(string input_file_str,string output_file_str);
	~CCrawl();

	//CCrawl��������Ҫ�ĺ���
	void DoCrawl();

	//����url�Լ��׽����ļ�������ץȡurl��Ӧ����ҳ
	void DownroadFile(CPSEFile *pse_file_ptr,CLinkForPSEFile *link_for_pse_file_ptr,
		CUrl iurl,int nGsock);

	//ÿ���̺߳���start�����õĺ���
	void Fetch(void *arg);

	//���url�������������뵽mapurl[�ȴ����ʵ�url]������ȥ
	void AddUrl(const char * url);

	//�õ��Ѿ����ʹ���URL��Ӧ��MD5ֵ������set_visited_url_md5��
	void GetVisitedUrlMd5();

	//�õ��Ѿ����ʹ���web��ҳ���Ӧ��MD5ֵ������set_visited_page_md5��
	void GetVisitedPageMd5();

	//�õ�������IP������map_ip_block������
	void GetIpBlock();

	//�õ����ɵ����������,����set_unreach_host_md5��
	void GetUnreachHostMd5();

	//�����е������
	void OpenFilesForOutput();

	//save in the process

	//��ץȡ����ҳ��PSE��ʽ�洢
	void SavePseRawData(CPSEFile *pse_file_ptr,CUrl *url_ptr,CPage *page_ptr);

	//��ץȡ����ҳ����ȡ��������Ϣ��������ҳ�ṹ��
	void SaveLinkForPseRawData(CLinkForPSEFile *link_for_pse_file_ptr,CUrl *url_ptr,CPage *page_ptr);

	//��ץȡ����ҳ?????????????????????????????????????????????????????????????????????????????????????????????????????????????
	void SaveIsamRawData(CUrl *url_ptr,CPage *page_ptr);

	//�����Ѿ����ʹ���url
	void SaveVisitedUrl(string url);

	//���治�ɵ����������
	void SaveUnreachHost(string host);

	//����Ϊ��������׼���ĳ�����Ϣ
	void SaveLinkForPSE(CPage *page_ptr);

	//???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
	bool SaveLink4SE031121(void *arg);

	//����Ϊ��ʷ��ҳ�浵׼���ĳ�������Ϣ
	void SaveLinkForHistory(CPage * page_ptr);

	//save while the process running
	
	//�����Ѿ����ʹ���Url��Ӧ��md5ֵ
	void SaveVisitedUrlMd5(string md5);

	//�����Ѿ����ʹ���page��Ӧ��md5ֵ
	void SaveVisitedPageMd5(string md5);

};


#endif