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
extern map<string,string> map_cache_host_lookup;//DNS缓存
extern vector<string> vector_unreach_host;
extern char **ParseRobot(char *data,char len);

set<string> set_visited_url_md5;//已经访问的URL对应的MD5值
set<string> set_visited_page_md5;//已经访问过的web网页体对应的MD5值
set<string> set_unvisited_url_md5;//没有访问过的URL对应的MD5值
set<string> set_unreach_host_md5;//不可达到的主机号对应的MD5值的集合

multimap<string, string, less<string> > replicas; //web网页体对应的MD5值<->web网页体对应的URL

//定义线层的初始变量并且初始化
HANDLE mutex_collection = CreateMutex(NULL,false,NULL);//保护map_urls资源
HANDLE mutex_unreach_host = CreateMutex(NULL,false,NULL);//保护set_unreach_host_md5&&ofs_unreach_host_file资源
HANDLE mutex_unvisited_url_md5 = CreateMutex(NULL,false,NULL);//保护set_unvisited_url_md5
HANDLE mutex_visited_url_md5 = CreateMutex(NULL,false,NULL);//保护set_visited_url_md5 && ofs_visited_url_md5_file
HANDLE mutex_visited_page_md5 = CreateMutex(NULL,false,NULL);//保护set_visited_page_md5 && ofs_visited_page_md5_file

HANDLE mutex_detect = CreateMutex(NULL,false,NULL);
HANDLE mutex_link_for_pse_file = CreateMutex(NULL,false,NULL);//保护ofs_link_for_pse_file
HANDLE mutex_link_for_history_file = CreateMutex(NULL,false,NULL);//保护ofs_link_for_history_file
HANDLE mutex_isam_file = CreateMutex(NULL,false,NULL);//保护ofs_isam_file
HANDLE mutex_visited_url_file = CreateMutex(NULL,false,NULL);//保护ofs_link_visited_url_file
HANDLE mutex_unvisited_host_file = CreateMutex(NULL,false,NULL);//保护ofs_unvisited_host_file
HANDLE mutex_replicas = CreateMutex(NULL,false,NULL);


map<unsigned long,unsigned long> map_ip_block;
bool b_f_over;//线程运行控制参数

multimap<string,string> map_urls;//保存没有访问的url的主机号<->对应的url

typedef map<unsigned long,unsigned long>::value_type val_type_ip_block;
typedef map<string,string>::value_type val_type;

void SaveReplicas( const char* file_name);//保存镜像网页对应的URL的一个值到指定的文件名中

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

//线程函数-->每个线程函数调用fetch(void*arg)函数
DWORD WINAPI start(void *arg)
{
	((CCrawl *)arg)->Fetch(arg);

	return 0;
}
 

/*这个函数在我们强制中断程序的时候，将map_urls
中没有访问完的url放入pse_unvisited_url.txt文件中,
扩充了我们的种子URL库*/
void SaveUnvisitedUrl()
{
	ofstream ofs_unvisited_url;
	ofs_unvisited_url.open(UNVISITED_FILE.c_str(),
		ios::in | ios::out | ios::trunc | ios::binary);//以二进制可追加的方式打开文件

	if(!ofs_unvisited_url)//打开文件失败
	{
		cerr<<"can not open the file "<<UNVISITED_FILE<<endl;
		exit(-1);
	}

	//将map_urls中没有访问完的url放入pse_unvisited.url文件中,扩充了我们的URL种子库
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

	int nGsock = -1;//之前的套接字文件描述符
	string strGHost;//之前的主机号

	//生成一个PSE file来存放网页数据
	//string ofs_name = DATA_PSE_FILE + "." + CStrFunction::itos(GetCurrentThreadId());//PSE.raw+当前线程号

	string ofs_name = DATA_PSE_FILE + CStrFunction::itos(GetCurrentThreadId())+ ".txt";//PSE+当前线程号+.txt
	CPSEFile pse_file(ofs_name);//创建一个PSE格式的文件，保存为原始网页库

	//生成一个link_for_pse file来存放链接数据
	ofs_name = DATA_LINK_FOR_PSE_FILE  + CStrFunction::itos(GetCurrentThreadId())+ ".txt";//PSE+当前线程号+.txt
	CLinkForPSEFile link_for_pse_file(ofs_name);//创建一个网页结构库

	int isleep_cnt = 0;//线程运行控制参数

	for(;;)
	{
		WaitForSingleObject(mutex_collection,INFINITE);//互斥锁

		int cnt = map_urls.size();
		if(cnt > 0)
		{
			//已经收集的没有访问的url
			cout<<"collection has "<<cnt<<" unvisited urls"<<endl;
			multimap<string,string>::iterator it = map_urls.begin();
			if(it != map_urls.end())
			{
				//从带访问的url队列中得到一个url进行访问
				str_url = (*it).second;
				map_urls.erase(it);

				ReleaseMutex(mutex_collection);

				//分解url
				CUrl iurl;
				//看看url是否有http://，没有则返回

				if(iurl.ParseUrl(str_url) == false)
				{
					cout<<"parse url false in Fetch"<<str_url<<endl;
					continue;
				}

				//表明现在抓取的网页所在的主机，同之前抓取的网页所在的主机不同
				//我们不能利用之前的套接字文件描述符进行CS通信,必须创建新的
				//套接字文件描述符进行通信,这是由于循环导致的
				if(strGHost != iurl.host_name)
				{
					closesocket(nGsock);
					nGsock = -1;
					strGHost = iurl.host_name;
				}

				//根据URL以及套接字文件描述符抓取URL对应的网页,并保存为原始网页库和网页结构库
				((CCrawl *)arg)->DownroadFile(&pse_file,&link_for_pse_file,iurl,nGsock);

				cnt = 0;
			}else	
			{
				ReleaseMutex(mutex_collection);
				
			}
		}else
		{
			//等待访问的url队列map_urls已经没有url了，这是我们需要挂起线程进行等待
			ReleaseMutex(mutex_collection);
			Sleep(1000);
			isleep_cnt++;
		}

		if(b_f_over == true && isleep_cnt == 200)//当线程挂起的次数达到两百的时候，结束调用fetch
		{
			break;
		}
	}

	pse_file.Close();
	link_for_pse_file.Close();

}

//函数：下载文件
void CCrawl::DownroadFile(CPSEFile *pse_file_ptr,CLinkForPSEFile *link_for_pse_file_ptr,
		CUrl iurl,int nGsock)
{
	char *downroaded_file = NULL,//网页体信息
		*file_header = NULL,//网页头信息
		*location = NULL;//网页重定向

	int file_length = 0;//网页体真实的字节长度
	string str_url_location = "";//保存网页的重定向超链接

	//之后请求的网页和之前请求的网页位于同一个主机上，我们可以利用之前
	//的套接字文件描述符进行通信，这样我们可以节约带宽，节省时间????为何？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？
	int nsock = nGsock;//将之前的套接字文件描述符赋值给nsock
	cout<<"PID = "<<GetCurrentThreadId()<<" nsock = "<<nsock<<endl;

	CHttp ihttp;
	//真正的抓取网页的函数，有了URL，搜集系统可以根据URL的标识抓取其对应的网页
	file_length = ihttp.Fetch(iurl.origin_url,&downroaded_file,&file_header,&location,&nsock);

	int location_count = 0;//标识url重定向的次数，如果重定向了3次，我们就不要抓取它对应的网页

	while(file_length == -300)//表明此时重定向了
	{
		//转到其他的地址
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

		//将获取到的重定向的URL给str_url_location,为下次抓取网页做准备
		str_url_location = location;
		if(location)
		{
			free(location);
			location = 0;
		}

		//因为重定向的URL可能是相对路径，所以我们必须将它转化为绝对路径
		//跟CPage类中提取超链接信息一样
		string::size_type index1 = CStrFunction::FindCase(str_url_location,"http");
		if(index1 != 0)//没有找到http协议
		{
			char c1 = iurl.origin_url.at(iurl.origin_url.length()-1);
			char c2 = str_url_location.at(0);

			if(c2 == '/')//重定向的url一定是相对url
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


	}//while循环结束

	nGsock = nsock;//将得到的套接字文件描述符给之前的套接字文件描述符，为下次重用做准备

	if(file_length == -1)//其他的各种错误,错误在CHttp的Fetch函数中
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

	if(file_length == -2)//在ip阻塞范围内
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

	if(file_length == -3)//错误的ip地址
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

	if(file_length == -4)//图片类型的网页
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
	
	//处理正常的网页，只要网页头或网页体的信息有一个为空，我们就认为网页不正常
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

	//将抓取的网页信息放入到CPage类中
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
	
	//解析网页头信息
	ipage.ParseHeaderInfo(ipage.header);

	if(ipage.connection_state == false)
	{
		closesocket(nGsock);
		nGsock = -1;
	}

	//过滤掉不是我们想要的网页体信息,注意？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？忽略了图片形式的网页
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

	//解压缩
	//如果是gzip编码，要解压缩，然后提取超链接信息
/*	char unzip_content_buffer[1024000];
	int unzip_length = 0;

	if(ipage.content_encoding == "gzip" && ipage.content_type == "text/html")
	{
		gzFile zip;
		string ofs_gzip_name;
		ofs_gzip_name = CStrFunction::itos(GetCurrentThreadId()) + ".gz";

		//以二进制截断的方式打开文件
		//ios::trunc 如果文件存在，则将文件长度截断为0，并清除文件的内容，如果文件不存在，则创建该文件
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

		//解压缩过程，将解压缩后的网页体信息放入到缓冲区域unzip_content_buffer
		unzip_length = gzread(zip,unzip_content_buffer,1024000);
		if(unzip_length == -1)
		{
			cout<<"read zip file "<<ofs_gzip_name.c_str()<<" error ."<<endl;
			exit(-1);
		}

		unzip_content_buffer[unzip_length] = 0;
		gzclose(zip);
	}//解压缩过程结束

	*/
	CMD5 imd5;
	string str_digest;

	//判断该URL是否在set_visited_url_md5中,在返回;不在加到其中，并保存
	imd5.GenerateMd5((unsigned char *)iurl.origin_url.c_str(),iurl.origin_url.length());//生成md5码
	str_digest = imd5.ToString();

	WaitForSingleObject(mutex_visited_url_md5,INFINITE);

	if(set_visited_url_md5.find(str_digest) != set_visited_url_md5.end())//表明已经抓取过
	{
		cout<<"the url :"<<iurl.origin_url.c_str()<<" have crawled !"<<endl;

		ReleaseMutex(mutex_visited_url_md5);
		return ;
	}

	//不在set_visited_url_md5中，现在必须插入set_visited_url_md5中
	//因为该URL现在已经访问过了
	set_visited_url_md5.insert(str_digest);
	SaveVisitedUrlMd5(str_digest);
	ReleaseMutex(mutex_visited_url_md5);

	//判断该网页体是否已经访问过,访问过返回,没有访问过加到set_visited_page_md5集合中
	imd5.GenerateMd5((unsigned char *)ipage.body_content.c_str(),ipage.body_content.length());
	str_digest = imd5.ToString();

	WaitForSingleObject(mutex_visited_page_md5,INFINITE);
	//网页体MD5同URL的关系插入到容器replicas中
	replicas.insert(pair<string,string>(str_digest,iurl.origin_url));

	if(set_visited_page_md5.find(str_digest) != set_visited_page_md5.end())//表明出现了镜像文件
	{
		cout<<"the page  have crawled !"<<endl;

		ReleaseMutex(mutex_visited_page_md5);
		return ;
	}

	//不在set_visited_page_md5中，现在必须插入set_visited_page_md5中
	//因为该URL现在已经访问过了
	set_visited_page_md5.insert(str_digest);
	SaveVisitedPageMd5(str_digest);
	ReleaseMutex(mutex_visited_page_md5);
	

	//将抓取到的网页以PSE格式放到原始网页库中
	SavePseRawData(pse_file_ptr,&iurl,&ipage);

	if(ipage.location.length()<1)
	{
		SaveVisitedUrl(iurl.origin_url);
	}
	else
		SaveVisitedUrl(ipage.location);

	if(ipage.content_type != "text/html")//只可以在text/html中发现超链接
		return ;

//=====================================保存单个网页的所有连接信息

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

//信号处理函数
static void SigTerm(int x)
{

	SaveUnvisitedUrl();
	SaveReplicas("repli");
	cout << "Terminated!" << endl;
	exit(0);
}

//得到已经访问过的URL对应的MD5值，放入set_visited_url_md5中
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


//得到已经访问过的web网页体对应的MD5值，放入setVisitedPageMD5中
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

//得到阻塞的ip,放入map_ip_block中
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

//得到不可达的主机号，放入set_unreach_host_md5中
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

//将抓取的文件以PSE的格式存储
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

//将抓取的网页从中提取超链接，建立网页结构库
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

//??????????????????????????????????关于ISAM的作用？？？？？？？？？？？？？？？？？？？
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

//保存访问过的url
void CCrawl::SaveVisitedUrl(string url)
{
	if(ofs_visited_url_file)
	{
		WaitForSingleObject(mutex_visited_url_file,INFINITE);

		ofs_visited_url_file<<url<<endl;

		ReleaseMutex(mutex_visited_url_file);
	}
}

//保存不可达的主机号
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

//保存网页链接
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

//保存为历史网页存档准备的超链接信息
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

//保存访问过的url_md5
void CCrawl::SaveVisitedUrlMd5(string md5)
{
	if(ofs_visited_url_md5_file)
	{
		ofs_visited_url_md5_file<<md5<<endl;
	}
}

//保存访问过的page_md5
void CCrawl::SaveVisitedPageMd5(string md5)
{
	if(ofs_visited_page_md5_file)
	{
		ofs_visited_page_md5_file<<md5<<endl;
	}
}

//打开所有的文件输出流
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

//CCrawl类的总控制函数
void CCrawl::DoCrawl()
{
	//set the signal function 作用？？？？？？？？？？？？？？？？
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

	//从文件中获取到其他的信息
	GetVisitedUrlMd5();
	GetVisitedPageMd5();
	GetIpBlock();
	GetUnreachHostMd5();
	

	//打开url种子库文件
	ifstream ifs_seed(input_file_name.c_str());
	
	if(!ifs_seed)
	{
		cout<<"can not open the url seed file "<<input_file_name<<endl;
		return ;
	}

	//打开文件输出流
	OpenFilesForOutput();

	//创建线程
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

	//获得未访问的url
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
	//等待所有的线程结束
	WaitForMultipleObjects(NUMBER_WORKERS,h_thread,false,INFINITE);
	//关闭所有的线程句柄
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

//将url放入map_urls到容器中
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

	//图片类型的网页，存放到历史网页链接库中
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

	//如果是阻塞的ip地址，剔除掉
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

	//确保同一个线程在一个网站上爬取
	int cnt = 0;

	for(;;)
	{
		if(1)//？？？？？？？
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