#include "Crawl.h"
#include "Url.h"
#include "MD5.h"



#include <zlib.h>
#include <set>
#include <Windows.h>
#include <stack>
#include <signal.h>
#include <time.h>

extern HANDLE my_mutex;
extern map<string,string> map_cache_host_lookup;//DNS����
extern vector<string> vector_unreach_host;
extern char **ParseRobot(char *data,char len);

set<string> set_visited_url_md5;//�Ѿ����ʵ�URL��Ӧ��MD5ֵ
set<string> set_visited_page_md5;//�Ѿ����ʹ���web��ҳ���Ӧ��MD5ֵ
set<string> set_unvisited_url_md5;//û�з��ʹ���URL��Ӧ��MD5ֵ
set<string> set_unreach_host_md5;//���ɴﵽ�������Ŷ�Ӧ��MD5ֵ�ļ���

multimap<string, string, less<string> > replicas; //web��ҳ���Ӧ��MD5ֵ<->web��ҳ���Ӧ��URL

//�����߲�ĳ�ʼ�������ҳ�ʼ��
HANDLE mutex_collection = CreateMutex(NULL,false,NULL);//����map_urls��Դ
HANDLE mutex_unreach_host = CreateMutex(NULL,false,NULL);//����set_unreach_host_md5&&ofs_unreach_host_file��Դ
HANDLE mutex_unvisited_url_md5 = CreateMutex(NULL,false,NULL);//����set_unvisited_url_md5
HANDLE mutex_visited_url_md5 = CreateMutex(NULL,false,NULL);//����set_visited_url_md5 && ofs_visited_url_md5_file
HANDLE mutex_visited_page_md5 = CreateMutex(NULL,false,NULL);//����set_visited_page_md5 && ofs_visited_page_md5_file

HANDLE mutex_detect = CreateMutex(NULL,false,NULL);
HANDLE mutex_link_for_pse_file = CreateMutex(NULL,false,NULL);//����ofs_link_for_pse_file
HANDLE mutex_link_for_history_file = CreateMutex(NULL,false,NULL);//����ofs_link_for_history_file
HANDLE mutex_isam_file = CreateMutex(NULL,false,NULL);//����ofs_isam_file
HANDLE mutex_visited_url_file = CreateMutex(NULL,false,NULL);//����ofs_link_visited_url_file
HANDLE mutex_unvisited_host_file = CreateMutex(NULL,false,NULL);//����ofs_unvisited_host_file
HANDLE mutex_replicas = CreateMutex(NULL,false,NULL);


map<unsigned long,unsigned long> map_ip_block;
bool b_f_over;//�߳����п��Ʋ���

multimap<string,string> map_urls;//����û�з��ʵ�url��������<->��Ӧ��url

typedef map<unsigned long,unsigned long>::value_type val_type_ip_block;
typedef map<string,string>::value_type val_type;

void SaveReplicas( const char* file_name);//���澵����ҳ��Ӧ��URL��һ��ֵ��ָ�����ļ�����

struct package
{
	CCrawl *crawl_ptr;
	CPage *page_ptr;
};

vector<string> vector_parsed_links;

/*int OnFind(const char *element,const char *attr, struct uri *uri,void *arg)
{
	struct package *p =(struct package *)arg;
	char buffer[URL_LEN+1];

	if(uri_combine(uri,buffer,URL_LEN+1,
		C_SCHEME | C_AUTHORITY | C_PATH | C_QUERY) > 0)
	{
		vector_parsed_links.push_back(buffer);
		if(!p->page_ptr->IsFilterLink(buffer))
		{
			//accept "a,link,flame,iframe,imag,area"
			if(strcmp(element,"imag") == 0)
			{
				WaitForSingleObject(mutex_link_for_history_file,INFINITE);

				if(p->crawl_ptr->ofs_link_for_history_file)
					p->crawl_ptr->ofs_link_for_history_file<<buffer<<endl;

				ReleaseMutex(mutex_link_for_history_file);
			}
			else
				p->crawl_ptr->AddUrl(buffer);
		}
	}

	uri_destroy(uri);
	free(uri);

	return 1;

}
*/

//�̺߳���-->ÿ���̺߳�������fetch(void*arg)����
DWORD WINAPI start(void *arg)
{
	((CCrawl *)arg)->Fetch(arg);

	return 0;
}
 

/*�������������ǿ���жϳ����ʱ�򣬽�map_urls
��û�з������url����pse_unvisited_url.txt�ļ���,
���������ǵ�����URL��*/
void SaveUnvisitedUrl()
{
	ofstream ofs_unvisited_url;
	ofs_unvisited_url.open(UNVISITED_FILE.c_str(),
		ios::in | ios::out | ios::trunc | ios::binary);//�Զ����ƿ�׷�ӵķ�ʽ���ļ�

	if(!ofs_unvisited_url)//���ļ�ʧ��
	{
		cerr<<"can not open the file "<<UNVISITED_FILE<<endl;
		exit(-1);
	}

	//��map_urls��û�з������url����pse_unvisited.url�ļ���,���������ǵ�URL���ӿ�
	multimap<string,string> ::iterator it;
	for(it=map_urls.begin();it != map_urls.end();it++)
	{
		ofs_unvisited_url<<(*it).second.c_str()<<"\n";
	}

	ofs_unvisited_url<<endl;
	ofs_unvisited_url.close();
}


//
void CCrawl::Fetch(void *arg)
{
	string str_url,host;

	int nGsock = -1;//֮ǰ���׽����ļ�������
	string strGHost;//֮ǰ��������

	//����һ��PSE file�������ҳ����
	//string ofs_name = DATA_PSE_FILE + "." + CStrFunction::itos(GetCurrentThreadId());//PSE.raw+��ǰ�̺߳�

	string ofs_name = DATA_PSE_FILE + CStrFunction::itos(GetCurrentThreadId())+ ".txt";//PSE+��ǰ�̺߳�+.txt
	CPSEFile pse_file(ofs_name);//����һ��PSE��ʽ���ļ�������Ϊԭʼ��ҳ��

	//����һ��link_for_pse file�������������
	ofs_name = DATA_LINK_FOR_PSE_FILE  + CStrFunction::itos(GetCurrentThreadId())+ ".txt";//PSE+��ǰ�̺߳�+.txt
	CLinkForPSEFile link_for_pse_file(ofs_name);//����һ����ҳ�ṹ��

	int isleep_cnt = 0;//�߳����п��Ʋ���

	for(;;)
	{
		WaitForSingleObject(mutex_collection,INFINITE);//������

		int cnt = map_urls.size();
		if(cnt > 0)
		{
			//�Ѿ��ռ���û�з��ʵ�url
			cout<<"collection has "<<cnt<<" unvisited urls"<<endl;
			multimap<string,string>::iterator it = map_urls.begin();
			if(it != map_urls.end())
			{
				//�Ӵ����ʵ�url�����еõ�һ��url���з���
				str_url = (*it).second;
				map_urls.erase(it);

				ReleaseMutex(mutex_collection);

				//�ֽ�url
				CUrl iurl;
				//����url�Ƿ���http://��û���򷵻�

				if(iurl.ParseUrl(str_url) == false)
				{
					cout<<"parse url false in Fetch"<<str_url<<endl;
					continue;
				}

				//��������ץȡ����ҳ���ڵ�������֮ͬǰץȡ����ҳ���ڵ�������ͬ
				//���ǲ�������֮ǰ���׽����ļ�����������CSͨ��,���봴���µ�
				//�׽����ļ�����������ͨ��,��������ѭ�����µ�
				if(strGHost != iurl.host_name)
				{
					closesocket(nGsock);
					nGsock = -1;
					strGHost = iurl.host_name;
				}

				//����URL�Լ��׽����ļ�������ץȡURL��Ӧ����ҳ,������Ϊԭʼ��ҳ�����ҳ�ṹ��
				((CCrawl *)arg)->DownroadFile(&pse_file,&link_for_pse_file,iurl,nGsock);

				cnt = 0;
			}else	
			{
				ReleaseMutex(mutex_collection);
				
			}
		}else
		{
			//�ȴ����ʵ�url����map_urls�Ѿ�û��url�ˣ�����������Ҫ�����߳̽��еȴ�
			ReleaseMutex(mutex_collection);
			Sleep(1000);
			isleep_cnt++;
		}

		if(b_f_over == true && isleep_cnt == 200)//���̹߳���Ĵ����ﵽ���ٵ�ʱ�򣬽�������fetch
		{
			break;
		}
	}

	pse_file.Close();
	link_for_pse_file.Close();

}

//�����������ļ�
void CCrawl::DownroadFile(CPSEFile *pse_file_ptr,CLinkForPSEFile *link_for_pse_file_ptr,
		CUrl iurl,int nGsock)
{
	char *downroaded_file = NULL,//��ҳ����Ϣ
		*file_header = NULL,//��ҳͷ��Ϣ
		*location = NULL;//��ҳ�ض���

	int file_length = 0;//��ҳ����ʵ���ֽڳ���
	string str_url_location = "";//������ҳ���ض�������

	//֮���������ҳ��֮ǰ�������ҳλ��ͬһ�������ϣ����ǿ�������֮ǰ
	//���׽����ļ�����������ͨ�ţ��������ǿ��Խ�Լ������ʡʱ��????Ϊ�Σ�������������������������������������������������������������
	int nsock = nGsock;//��֮ǰ���׽����ļ���������ֵ��nsock
	cout<<"PID = "<<GetCurrentThreadId()<<" nsock = "<<nsock<<endl;

	CHttp ihttp;
	//������ץȡ��ҳ�ĺ���������URL���Ѽ�ϵͳ���Ը���URL�ı�ʶץȡ���Ӧ����ҳ
	file_length = ihttp.Fetch(iurl.origin_url,&downroaded_file,&file_header,&location,&nsock);

	int location_count = 0;//��ʶurl�ض���Ĵ���������ض�����3�Σ����ǾͲ�Ҫץȡ����Ӧ����ҳ

	while(file_length == -300)//������ʱ�ض�����
	{
		//ת�������ĵ�ַ
		if( strlen(location)> URL_LEN -1 || location_count == 3 ||strlen(location) == 0)
		{
			if(location)
			{
				free(location);
				location = 0;
			}
			file_length = -1;
			break;
		}

		//����ȡ�����ض����URL��str_url_location,Ϊ�´�ץȡ��ҳ��׼��
		str_url_location = location;
		if(location)
		{
			free(location);
			location = 0;
		}

		//��Ϊ�ض����URL���������·�����������Ǳ��뽫��ת��Ϊ����·��
		//��CPage������ȡ��������Ϣһ��
		string::size_type index1 = CStrFunction::FindCase(str_url_location,"http");
		if(index1 != 0)//û���ҵ�httpЭ��
		{
			char c1 = iurl.origin_url.at(iurl.origin_url.length()-1);
			char c2 = str_url_location.at(0);

			if(c2 == '/')//�ض����urlһ�������url
				str_url_location = "http://" + iurl.host_name + str_url_location;
			else if(c1 != '/' && c2 != '/')
			{
				string::size_type index;
				index = iurl.origin_url.rfind('/');
				if(index != string::npos)
				{
					if(index > 6)
					{
						str_url_location = iurl.origin_url.substr(0,index+1) + str_url_location;
					}
					else
						str_url_location = iurl.origin_url + "/" + str_url_location;
				}
				else
				{
					file_length = -1;
					break;
				}
			}
			else
			{
				if(c1 == '/')
					str_url_location = iurl.origin_url + str_url_location;
				else
					str_url_location = iurl.origin_url + "/" + str_url_location;
			}

		}

		CPage ipage;
		if(ipage.IsFilterLink(str_url_location))
		{
			file_length = -1;
			break;
		}

		cout<<"PID= "<<GetCurrentThreadId()<<" sock= "<<nGsock<<endl;
		file_length = ihttp.Fetch(str_url_location,&downroaded_file,&file_header,&location,&nGsock);
		location_count++;


	}//whileѭ������

	nGsock = nsock;//���õ����׽����ļ���������֮ǰ���׽����ļ���������Ϊ�´�������׼��

	if(file_length == -1)//�����ĸ��ִ���,������CHttp��Fetch������
	{
		cout<<"error in the page of"<<iurl.origin_url<<endl;
		
		if(file_header)
		{
			free(file_header);
			file_header = NULL;
		}

		if(downroaded_file)
		{
			free(downroaded_file);
			downroaded_file = NULL;
		}

		cout<<"unreach host : "<<iurl.host_name<<endl;
		return ;
	}

	if(file_length == -2)//��ip������Χ��
	{
		if(file_header)
		{
			free(file_header);
			file_header = NULL;
		}

		if(downroaded_file)
		{
			free(downroaded_file);
			downroaded_file = NULL;
		}

		SaveUnreachHost(iurl.host_name);
		cout<<"out of block host : "<<iurl.host_name<<endl;
		return ;
	}

	if(file_length == -3)//�����ip��ַ
	{
		if(file_header)
		{
			free(file_header);
			file_header = NULL;
		}

		if(downroaded_file)
		{
			free(downroaded_file);
			downroaded_file = NULL;
		}

		cout<<"invalid host : "<<iurl.host_name<<endl;
		return ;
	}

	if(file_length == -4)//ͼƬ���͵���ҳ
	{
		if(file_header)
		{
			free(file_header);
			file_header = NULL;
		}

		if(downroaded_file)
		{
			free(downroaded_file);
			downroaded_file = NULL;
		}

		if(ofs_link_for_history_file)
		{
			
		}
		cout<<"image host : "<<iurl.host_name<<endl;
		return ;
	}
	
	//������������ҳ��ֻҪ��ҳͷ����ҳ�����Ϣ��һ��Ϊ�գ����Ǿ���Ϊ��ҳ������
	if(!file_header || !downroaded_file)
	{
		if(file_header)
		{
			free(file_header);
			file_header = NULL;
		}

		if(downroaded_file)
		{
			free(downroaded_file);
			downroaded_file = NULL;
		}

		closesocket(nGsock);
		nGsock = -1;
		cout<<"not the nomal host"<<endl;
		return ;
	}

	//��ץȡ����ҳ��Ϣ���뵽CPage����
	CPage ipage(iurl.origin_url,str_url_location,file_header,downroaded_file,file_length);

	if(file_header)
	{
		free(file_header);
		file_header = NULL;
	}

	if(downroaded_file)
	{
		free(downroaded_file);
		downroaded_file = NULL;
	}
	
	//������ҳͷ��Ϣ
	ipage.ParseHeaderInfo(ipage.header);

	if(ipage.connection_state == false)
	{
		closesocket(nGsock);
		nGsock = -1;
	}

	//���˵�����������Ҫ����ҳ����Ϣ,ע�⣿����������������������������������������������������������������������ͼƬ��ʽ����ҳ
	if(ipage.content_type != "text/html"
		&&ipage.content_type != "text/plain"
		&&ipage.content_type != "text/xml"
		&&ipage.content_type != "application/msword"
		&&ipage.content_type != "applicaiton/pdf"
		&&ipage.content_type != "text/rtf"
		&&ipage.content_type != "applicaiton/postscript"
		&&ipage.content_type != "applicaiton/vnd.ms-excel"
		&&ipage.content_type != "application/vnd.ms-powerpoint")
	{
		cout<<"unwant host type"<<iurl.host_name<<endl;
		return ;
	}

	//��ѹ��
	//�����gzip���룬Ҫ��ѹ����Ȼ����ȡ��������Ϣ
/*	char unzip_content_buffer[1024000];
	int unzip_length = 0;

	if(ipage.content_encoding == "gzip" && ipage.content_type == "text/html")
	{
		gzFile zip;
		string ofs_gzip_name;
		ofs_gzip_name = CStrFunction::itos(GetCurrentThreadId()) + ".gz";

		//�Զ����ƽضϵķ�ʽ���ļ�
		//ios::trunc ����ļ����ڣ����ļ����Ƚض�Ϊ0��������ļ������ݣ�����ļ������ڣ��򴴽����ļ�
		ofstream ofs_downroad_file(ofs_gzip_name.c_str(),ios::trunc|ios::binary);

		cout<<"file length : "<<endl;
		ofs_downroad_file.write(ipage.body_content.c_str(),ipage.body_content_length);
		ofs_downroad_file.close();

		zip = gzopen(ofs_gzip_name.c_str(),"rb");

		if(zip == NULL)
		{
			cout<<"open zip file "<<ofs_gzip_name.c_str()<<" error ."<<endl;
			exit(-1);

		}

		//��ѹ�����̣�����ѹ�������ҳ����Ϣ���뵽��������unzip_content_buffer
		unzip_length = gzread(zip,unzip_content_buffer,1024000);
		if(unzip_length == -1)
		{
			cout<<"read zip file "<<ofs_gzip_name.c_str()<<" error ."<<endl;
			exit(-1);
		}

		unzip_content_buffer[unzip_length] = 0;
		gzclose(zip);
	}//��ѹ�����̽���

	*/
	CMD5 imd5;
	string str_digest;

	//�жϸ�URL�Ƿ���set_visited_url_md5��,�ڷ���;���ڼӵ����У�������
	imd5.GenerateMd5((unsigned char *)iurl.origin_url.c_str(),iurl.origin_url.length());//����md5��
	str_digest = imd5.ToString();

	WaitForSingleObject(mutex_visited_url_md5,INFINITE);

	if(set_visited_url_md5.find(str_digest) != set_visited_url_md5.end())//�����Ѿ�ץȡ��
	{
		cout<<"the url :"<<iurl.origin_url.c_str()<<" have crawled !"<<endl;

		ReleaseMutex(mutex_visited_url_md5);
		return ;
	}

	//����set_visited_url_md5�У����ڱ������set_visited_url_md5��
	//��Ϊ��URL�����Ѿ����ʹ���
	set_visited_url_md5.insert(str_digest);
	SaveVisitedUrlMd5(str_digest);
	ReleaseMutex(mutex_visited_url_md5);

	//�жϸ���ҳ���Ƿ��Ѿ����ʹ�,���ʹ�����,û�з��ʹ��ӵ�set_visited_page_md5������
	imd5.GenerateMd5((unsigned char *)ipage.body_content.c_str(),ipage.body_content.length());
	str_digest = imd5.ToString();

	WaitForSingleObject(mutex_visited_page_md5,INFINITE);
	//��ҳ��MD5ͬURL�Ĺ�ϵ���뵽����replicas��
	replicas.insert(pair<string,string>(str_digest,iurl.origin_url));

	if(set_visited_page_md5.find(str_digest) != set_visited_page_md5.end())//���������˾����ļ�
	{
		cout<<"the page  have crawled !"<<endl;

		ReleaseMutex(mutex_visited_page_md5);
		return ;
	}

	//����set_visited_page_md5�У����ڱ������set_visited_page_md5��
	//��Ϊ��URL�����Ѿ����ʹ���
	set_visited_page_md5.insert(str_digest);
	SaveVisitedPageMd5(str_digest);
	ReleaseMutex(mutex_visited_page_md5);
	

	//��ץȡ������ҳ��PSE��ʽ�ŵ�ԭʼ��ҳ����
	SavePseRawData(pse_file_ptr,&iurl,&ipage);

	if(ipage.location.length()<1)
	{
		SaveVisitedUrl(iurl.origin_url);
	}
	else
		SaveVisitedUrl(ipage.location);

	if(ipage.content_type != "text/html")//ֻ������text/html�з��ֳ�����
		return ;

//=====================================���浥����ҳ������������Ϣ

	if (ipage.ParseHyperLinks() == false)
	{
		return;
	}
	
	SaveLinkForPSE( &ipage);
	SaveLinkForHistory( &ipage);

	map<string,string>::iterator it = ipage.map_link_for_pse.begin();
	string str;
	for( ; it!= ipage.map_link_for_pse.end(); ++it )
	{
		str = (*it).first;
		AddUrl( str.c_str() );

	}

//========================================

	return ;
}

void SaveReplicas(const char *file_name)
{
	ofstream ofs(file_name, ios::out | ios::binary | ios::app);

	if(!ofs)
	{
		cout<<"can not open file : "<<file_name<<endl;
	}
	string md5;

	WaitForSingleObject(mutex_replicas,INFINITE);

	multimap<string,string,less<string>>::const_iterator it;
	ostringstream *oss = 0;
	int i=1;
	for(it = replicas.begin();it != replicas.end();it++)
	{
		if(!md5.empty() && md5 != (*it).first)
		{
			if(i>=2)
				ofs<<(*oss).str()<<endl;
			delete oss;

			oss = new ostringstream;
			(*oss)<<it->first<<endl;
			i = 0;
			md5 = it->first;
		}
		else if(md5.empty())
		{
			md5 = it->first;
			oss = new ostringstream;
			(*oss)<<it->first<<endl;
			i = 0;
			
		}

		if(oss != 0)
		{
			(*oss)<<it->second<<endl;
		}
		i++;

	}

	ReleaseMutex(mutex_replicas);
}

CCrawl::CCrawl()
{

}

CCrawl::CCrawl(string input_file_name, string output_file_name)
{
	this->input_file_name = input_file_name;
	this->output_file_name = output_file_name;
}

CCrawl::~CCrawl()
{
	ofs_visited_url_file.close();
	ofs_link_for_pse_file.close();
	ofs_link_for_history_file.close();
	isam_file.Close();
	ofs_visited_url_md5_file.close();
	ofs_visited_page_md5_file.close();
}

//�źŴ�����
static void SigTerm(int x)
{

	SaveUnvisitedUrl();
	SaveReplicas("repli");
	cout << "Terminated!" << endl;
	exit(0);
}

//�õ��Ѿ����ʹ���URL��Ӧ��MD5ֵ������set_visited_url_md5��
void CCrawl::GetVisitedUrlMd5()
{
	ifstream ifs_md5(URL_MD5_FILE.c_str(),ios::binary);
	if(!ifs_md5)
		return;

	string str_md5;
	while(getline(ifs_md5,str_md5))
		set_visited_url_md5.insert(str_md5);

	ifs_md5.close();
	cout<<"have got "<<set_visited_url_md5.size()<< " md5 values of visited url"<<endl;

}


//�õ��Ѿ����ʹ���web��ҳ���Ӧ��MD5ֵ������setVisitedPageMD5��
void CCrawl::GetVisitedPageMd5()
{
	ifstream ifs_md5(PAGE_MD5_FILE.c_str(),ios::binary);
	if(!ifs_md5)
		return;

	string str_md5;
	while(getline(ifs_md5,str_md5))
		set_visited_page_md5.insert(str_md5);

	ifs_md5.close();
	cout<<"have got "<<set_visited_page_md5.size()<< " md5 values of visited page"<<endl;
}

//�õ�������ip,����map_ip_block��
void CCrawl::GetIpBlock()
{
	ifstream ifs_ip_block(IP_BLOCK_FILE.c_str(),ios::binary);
	if(!ifs_ip_block)
	{
		cout<<"can not open the file PseIpBlock"<<endl;
		return;
	}

	string str_ip_block;
	while(getline(ifs_ip_block,str_ip_block))
	{
		if(str_ip_block[0] == '\0' ||str_ip_block[0] == '#'
			||str_ip_block[0] == '\n')
			continue;

		char buf1[64],buf2[64];
		buf1[0] = '\0';
		buf2[0] = '\0';

		sscanf(str_ip_block.c_str(),"%s %s",buf1,buf2);
		map_ip_block.insert(val_type_ip_block(inet_addr(buf1),inet_addr(buf2)));

	}
	ifs_ip_block.close();
		
}

//�õ����ɴ�������ţ�����set_unreach_host_md5��
void CCrawl::GetUnreachHostMd5()
{
	ifstream ifs_unreach_host(UNREACH_HOST_FILE.c_str(),ios::binary);
	if(!ifs_unreach_host)
	{
		cout<<"open the file"<<UNREACH_HOST_FILE.c_str()<<" error "<<endl;
		return;
	}

	string str_unreach_host;
	while(getline(ifs_unreach_host,str_unreach_host))
	{
		if(str_unreach_host[0] == '\0' ||str_unreach_host[0] == '#'
			||str_unreach_host[0] == '\n')
			continue;

		CStrFunction::StrToLower(str_unreach_host,str_unreach_host.size());

		CMD5 imd5;
		imd5.GenerateMd5((unsigned char *)str_unreach_host.c_str(),str_unreach_host.size());

		string str_digest = imd5.ToString();
		set_unreach_host_md5.insert(str_digest);
	}

	ifs_unreach_host.close();
}

//��ץȡ���ļ���PSE�ĸ�ʽ�洢
void CCrawl::SavePseRawData(CPSEFile *pse_file_ptr,CUrl *url_ptr,CPage *page_ptr)
{
	if(!pse_file_ptr || !url_ptr || !page_ptr)
	{
		cout<<"the pse file ptr , url ptr ,page ptr is empty"<<endl;
		return ;
	}

	file_arg arg;
	arg.url_ptr = url_ptr;
	arg.page_ptr = page_ptr;

	pse_file_ptr->Write((void *) &arg);
}

//��ץȡ����ҳ������ȡ�����ӣ�������ҳ�ṹ��
void CCrawl::SaveLinkForPseRawData(CLinkForPSEFile *link_for_pse_file_ptr,CUrl *url_ptr,CPage *page_ptr)
{
	if(!link_for_pse_file_ptr || !url_ptr || page_ptr)
	{
		cout<<"the link_for_pse_file_ptrr , url ptr ,page ptr is empty"<<endl;
		return ;
	}

	file_arg arg;
	arg.url_ptr = url_ptr;
	arg.page_ptr = page_ptr;

	link_for_pse_file_ptr->Write((void *) &arg);
}

//??????????????????????????????????����ISAM�����ã�������������������������������������
/*void CCrawl::SaveIsamRawData(CUrl *url_ptr,CPage *page_ptr)
{
	if(!url_ptr || page_ptr)
	{
		cout<<"url ptr ,page ptr is empty in save ISAM data"<<endl;
		return ;
	}

	file_arg arg;
	arg.url_ptr = url_ptr;
	arg.page_ptr = page_ptr;

	WaitForSingleObject(mutex_isam_file,INFINITE);

	isam_file.Write((void *) &arg);

	ReleaseMutex(mutex_isam_file);
}*/

//������ʹ���url
void CCrawl::SaveVisitedUrl(string url)
{
	if(ofs_visited_url_file)
	{
		WaitForSingleObject(mutex_visited_url_file,INFINITE);

		ofs_visited_url_file<<url<<endl;

		ReleaseMutex(mutex_visited_url_file);
	}
}

//���治�ɴ��������
void CCrawl::SaveUnreachHost(string host)
{
	CMD5 imd5;
	imd5.GenerateMd5((unsigned char *)host.c_str(),host.size());

	string str_digest = imd5.ToString();

	if(set_unreach_host_md5.find(str_digest) == set_unreach_host_md5.end())
	{
		WaitForSingleObject(mutex_unreach_host,INFINITE);

		set_unreach_host_md5.insert(str_digest);
		if(ofs_unreached_host_file)
		{
			ofs_unreached_host_file<<host<<endl;

		}

		ReleaseMutex(mutex_unreach_host);
	}
}

//������ҳ����
void CCrawl::SaveLinkForPSE(CPage *page_ptr)
{
	if(ofs_link_for_pse_file && page_ptr->page_link_number>0)
	{
		WaitForSingleObject(mutex_link_for_pse_file,INFINITE);

		ofs_link_for_pse_file<<"root_url: "<<page_ptr->url<<endl;
		ofs_link_for_pse_file<<"charset: "<<page_ptr->char_set<<endl;
		ofs_link_for_pse_file<<"number: "<<page_ptr->page_link_number<<endl;
		ofs_link_for_pse_file<<"link_anchortext: "<<endl;

		map<string,string>::iterator it;
		for(it = page_ptr->map_link_for_pse.begin();it != page_ptr->map_link_for_pse.end();it++)
		{
			ofs_link_for_pse_file<<(*it).first<<'\t'<<(*it).second<<endl;
		}

		ReleaseMutex(mutex_link_for_pse_file);
	}
}

/*
void CCrawl::SaveLink4SE031121(void *arg)
{
	/////////
}
*/

//����Ϊ��ʷ��ҳ�浵׼���ĳ�������Ϣ
void CCrawl::SaveLinkForHistory(CPage * page_ptr)
{
	if(ofs_link_for_history_file && page_ptr->history_page_link_number>0)
	{
		WaitForSingleObject(mutex_link_for_history_file,INFINITE);

		vector<string>::iterator it;
		for(it = page_ptr->vector_link_for_history.begin();it != page_ptr->vector_link_for_history.end();it++)
		{
			string s = (*it);
			ofs_link_for_history_file<<s<<"+"<<page_ptr->url<<"$";	
		}

		ReleaseMutex(mutex_link_for_history_file);
	}
}

//������ʹ���url_md5
void CCrawl::SaveVisitedUrlMd5(string md5)
{
	if(ofs_visited_url_md5_file)
	{
		ofs_visited_url_md5_file<<md5<<endl;
	}
}

//������ʹ���page_md5
void CCrawl::SaveVisitedPageMd5(string md5)
{
	if(ofs_visited_page_md5_file)
	{
		ofs_visited_page_md5_file<<md5<<endl;
	}
}

//�����е��ļ������
void CCrawl::OpenFilesForOutput()
{
	//open ISAM file for output
	//isam_file.Open(DATA_FILE_NAME,INDEX_FILE_NAME);

	//open visited.url file for output
	ofs_visited_url_file.open(output_file_name.c_str(),
		ios::out | ios::app |ios::binary);
	if(!ofs_visited_url_file)
	{
		cerr<<"can not open "<<VISITED_FILE<<"for output"<<endl;
		return ;
	}

	//open link_for_pse.url file for output
	ofs_link_for_pse_file.open(LINK_FOR_PSE_FILE.c_str(),
		ios::out | ios::app |ios::binary);
	if(!ofs_link_for_pse_file)
	{
		cerr<<"can not open "<<LINK_FOR_PSE_FILE<<"for output"<<endl;
		return ;
	}

	//open link_for_history.url file for output
	ofs_link_for_history_file.open(LINK_FOR_HISTORY_FILE.c_str(),
		ios::out | ios::app |ios::binary);
	if(!ofs_link_for_history_file)
	{
		cerr<<"can not open "<<LINK_FOR_HISTORY_FILE<<"for output"<<endl;
		return ;
	}

	//open unreach_host file for output
	ofs_unreached_host_file.open(UNREACH_HOST_FILE.c_str(),
		ios::out | ios::app |ios::binary);
	if(!ofs_unreached_host_file)
	{
		cerr<<"can not open "<<UNREACH_HOST_FILE<<"for output"<<endl;
		return ;
	}

	//open visited url md5 file for output
	ofs_visited_url_md5_file.open(URL_MD5_FILE.c_str(),
		ios::out | ios::app |ios::binary);
	if(!ofs_visited_url_md5_file)
	{
		cerr<<"can not open "<<URL_MD5_FILE<<"for output"<<endl;
		return ;
	}

	//open visited page md5 file for output
	ofs_visited_page_md5_file.open(PAGE_MD5_FILE.c_str(),
		ios::out | ios::app |ios::binary);
	if(!ofs_visited_page_md5_file)
	{
		cerr<<"can not open "<<PAGE_MD5_FILE<<"for output"<<endl;
		return ;
	}

}

//CCrawl����ܿ��ƺ���
void CCrawl::DoCrawl()
{
	//set the signal function ���ã�������������������������������
/*	signal(SIGTERM,SigTerm);
	signal(SIGILL,SigTerm);
	signal(SIGFPE,SigTerm);
	signal(SIGINT,SIG_IGN);
	signal(SIGBREAK,SIG_IGN);*/

	//output the begin time
	char str_time[128];
	time_t t_date;
	memset(str_time,0,128);
	time(&t_date);
	strftime(str_time,128,"%a, %d %b %Y %H:%M:%S GMT",localtime(&t_date));

	cout << "\n\nBegin at: " << str_time << "\n\n";

	//���ļ��л�ȡ����������Ϣ
	GetVisitedUrlMd5();
	GetVisitedPageMd5();
	GetIpBlock();
	GetUnreachHostMd5();
	

	//��url���ӿ��ļ�
	ifstream ifs_seed(input_file_name.c_str());
	
	if(!ifs_seed)
	{
		cout<<"can not open the url seed file "<<input_file_name<<endl;
		return ;
	}

	//���ļ������
	OpenFilesForOutput();

	//�����߳�
	DWORD dw_thread_id[NUMBER_WORKERS];
	HANDLE h_thread[NUMBER_WORKERS];

	for(int i = 0;i<NUMBER_WORKERS;i++)
	{
		h_thread[i] = CreateThread(NULL,0,start,this,0,&dw_thread_id[i]);

		if(h_thread[i] == NULL)
		{
			cout<<"can not create the thread "<<i<<endl;
			ExitProcess(i);
		}
	}


	string str_url;
	CPage ipage;

	while(getline(ifs_seed,str_url))
	{
		string::size_type index;

		if(str_url[0] == '\0' ||str_url[0] == '#'
			|| str_url[0] == '\n')
			continue;

		index = str_url.find('\t');
		if(index != string::npos)
		{
			str_url = str_url.substr(0,index);
		}

		index = CStrFunction::FindCase(str_url,"http");
		if(index == string::npos)
		{
			index = str_url.find('/');
			if(index == string::npos)
			{
				str_url = "http://" + str_url + "/";
			}
			else
				str_url = "http://" + str_url;
		}

		if(ipage.IsFilterLink(str_url))
			continue;
		AddUrl(str_url.c_str());
	}

	//���δ���ʵ�url
	ifstream ifs_unvisited_url(UNVISITED_FILE.c_str(),ios::binary);
	if(!ifs_unvisited_url)
	{
		while(getline(ifs_unvisited_url,str_url))
		{
			string::size_type index;

			if(str_url[0] == '\0' ||str_url[0] == '#'
				|| str_url[0] == '\n')
				continue;

			index = str_url.find('\t');
			if(index != string::npos)
			{
				str_url = str_url.substr(0,index);
			}

			if(ipage.IsFilterLink(str_url))
				continue;

			AddUrl(str_url.c_str());
				
		}
	}
	else
		cout<<"can not open the file : "<<UNVISITED_FILE<<endl;

	b_f_over = true;
	
	cout<<"finished to get all unvisited urls"<<endl;
	//�ȴ����е��߳̽���
	WaitForMultipleObjects(NUMBER_WORKERS,h_thread,false,INFINITE);
	//�ر����е��߳̾��
	for(int i = 0;i<NUMBER_WORKERS;i++)
	{
		CloseHandle(h_thread[i]);
	}
	
	cout<<"all of "<<NUMBER_WORKERS<<" thread have been closed"<<endl;

	SaveUnvisitedUrl();
	SaveReplicas("repli");

	memset(str_time,0,128);
	time(&t_date);
	strftime(str_time, 128, "%a, %d %b %Y %H:%M:%S GMT", localtime(&t_date));
	cout << "\n\nEnd at: " << str_time << "\n\n";

}

//��url����map_urls��������
void CCrawl::AddUrl(const char * url)
{
	string str_url = url;
	if(str_url.empty() || str_url.length() < 8)
	{
		cout<<"the url is empty or too short"<<endl;
		return ;
	}

	CPage ipage;
	if(ipage.NormalizeUrl(str_url) == false)
		return ;

	CUrl iurl;

	//ͼƬ���͵���ҳ����ŵ���ʷ��ҳ���ӿ���
	if(iurl.IsImageUrl(str_url))
	{
		if(ofs_link_for_history_file)
		{
			WaitForSingleObject(mutex_link_for_history_file,INFINITE);

			ofs_link_for_history_file<<str_url<<endl;

			ReleaseMutex(mutex_link_for_history_file);
		}

		return ;
	}

	if(iurl.ParseUrl(str_url) == false)
	{
		cout<<"parse url error in AddUrl"<<endl;
		return ;
	}

	if(iurl.IsValidHost(iurl.host_name.c_str()) == false)
	{
		cout<<"not the valid host in AddUrl"<<endl;
		return ;
	}

	if(iurl.IsForeignHost(iurl.host_name.c_str()) )
	{
		cout<<"foreign host in AddUrl"<<endl;
		return ;
	}

	//�����������ip��ַ���޳���
	unsigned long inaddr = 0;
	char *ip = NULL;

	inaddr =(unsigned long) inet_addr(iurl.host_name.c_str());

	if(inaddr != INADDR_NONE)
	{
		ip = new char[iurl.host_name.size() + 1];
		memset(ip,0,iurl.host_name.size() + 1);
		memcpy(ip,iurl.host_name.c_str(),iurl.host_name.size());
		if(!iurl.IsValidIp(ip))
		{
			delete []ip;
			ip = NULL;
			return ;
		}

		delete []ip;
		ip = NULL;
	}


	CStrFunction::StrToLower(iurl.host_name,iurl.host_name.size());

	CMD5 imd5;
	imd5.GenerateMd5((unsigned char *)str_url.c_str(),str_url.size());

	string str_digest = imd5.ToString();

	if(set_visited_url_md5.find(str_digest) != set_visited_url_md5.end())
	{
		return ;
	}

	if(set_unvisited_url_md5.find(str_digest) != set_unvisited_url_md5.end())
	{
		return ;
	}
	else
	{
		WaitForSingleObject(mutex_unvisited_url_md5,INFINITE);
		set_unvisited_url_md5.insert(str_digest);
		ReleaseMutex(mutex_unvisited_url_md5);
	}

	//ȷ��ͬһ���߳���һ����վ����ȡ
	int cnt = 0;

	for(;;)
	{
		if(1)//��������������
		{
			WaitForSingleObject(mutex_visited_url_md5,INFINITE);
			map_urls.insert(val_type(iurl.host_name,str_url));
			ReleaseMutex(mutex_visited_url_md5);
			break;
		}
		else
		{
			cnt++;
			if(cnt%100 == 0)
				cout<<"~";
			

			if(cnt == 5000)
			{
				cout<<"remove it"<<endl;
			}

			Sleep(4000);
		}
	}

}