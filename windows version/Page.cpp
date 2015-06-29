#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <iterator>


#include "Page.h"
#include "StrFunction.h"
#include "Url.h"

using namespace std;

CPage::CPage()
{
	//初始化成员变量
	status_code = 0;
	header_content_length = 0;
	connection_state = false;
	location = "";
	content_encoding = "";
	content_type = "";
	char_set = "";
	transfer_encoding = "";
	my_page_type = PLAIN_TEXT;


	body_content = "";
	body_content_length = 0;
	content_no_tags = "";
	content_link_info = "";


	link_for_search = "";
	link_for_search_length = 0;


	link_for_history = "";
	link_for_history_length = 0;


	page_link_number = 0;
	history_page_link_number = 0;


	//超链接信息以及超链接的描述信息初始化都为空
	for(int i=0;i<MAX_URL_REFERENCES;i++)
	{
		page_link[i].anchor_text = NULL;
		page_link[i].link        = NULL;
		page_link[i].str_char_set= "";

		if(i<MAX_URL_REFERENCES/2)
		{
			history_page_link[i].link = NULL;
		}
	}

}

CPage::CPage(string str_url , string str_location , char *header , char *body , int body_length)
{
	//初始化成员变量
	status_code = 0;
	header_content_length = 0;
	connection_state = false;
	location = "";
	content_encoding = "";
	content_type = "";
	char_set = "";
	transfer_encoding = "";
	my_page_type = PLAIN_TEXT;


	body_content = "";
	body_content_length = 0;
	content_no_tags = "";
	content_link_info = "";


	link_for_search = "";
	link_for_search_length = 0;


	link_for_history = "";
	link_for_history_length = 0;


	page_link_number = 0;
	history_page_link_number = 0;


	//超链接信息以及超链接的描述信息初始化都为空
	for(int i=0;i<MAX_URL_REFERENCES;i++)
	{
		page_link[i].anchor_text = NULL;
		page_link[i].link        = NULL;
		page_link[i].str_char_set= "";

		if(i<MAX_URL_REFERENCES/2)
		{
			history_page_link[i].link = NULL;
		}
	}

	//将传进的参数进行赋值
	this->url = str_url;
	this->location = str_location;
	this->header = header;
	this->header_length = strlen(header);
	this->body_content.assign(body,body_length);
	this->body_content_length = body_length;
}

CPage::~CPage()
{

}

void CPage::ParseHeaderInfo(string header)
{
	GetStatusCode(header);
	GetContentLength(header);
	GetConnectionState(header);
	GetLocation(header);
	GetCharset(header);
	GetContentEncoding(header);
	GetContentType(header);
	GetTransferEncoding(header);
}

//得到状态码
void CPage::GetStatusCode(string header)
{
	CStrFunction::StrToLower(header , header.length());

	char *char_index = (char *)strstr(header.c_str(),"http/");
	if(char_index == NULL)
	{
		this->status_code = -1;
		return ;
	}
	while(*char_index != ' ')
	{
		char_index++;
	}
	char_index++;

	int ret = sscanf(char_index, "%i", &this->status_code);//格式化字符串输入
	if(ret != 1)
		this->status_code = -1;

}

//从网页头信息中提取的网页体的长度
void CPage::GetContentLength(string header)
{
	CStrFunction::StrToLower(header , header.length());

	char *char_index = (char *)strstr(header.c_str(),"content-length");
	if(char_index == NULL)
	{
		return ;
	}
	while(*char_index != ' ')
	{
		char_index++;
	}
	char_index++;

	int ret = sscanf(char_index, "%i", &this->header_content_length);//格式化字符串输入
	if(ret != 1)
		this->header_content_length = -1;
}

//得到重定向信息
void CPage::GetLocation(string header)
{
	string::size_type pre_index,index;
	const string delims("\r\n");
	string str = header;

	CStrFunction::StrToLower(header,header.length());
	index = header.find("location:");

	if(index != string::npos)
	{
		pre_index = index + sizeof("location: ") -1;
		index = header.find_first_of(delims,pre_index);
		if(index != string::npos)
		{
			this->location = str.substr(pre_index,index-pre_index);
		}
	}

}


//得到字符集
void CPage::GetCharset(string header)
{
	string::size_type pre_index,index;
	const string delims(" \",;>");

	CStrFunction::StrToLower(header, header.size());

	index = header.find("charset=");
	if( index != string::npos) {
		this->char_set = header.substr(index + sizeof("charset=") -1);
	}

	header = this->body_content;
	header = header.substr(0,2024) ;
	CStrFunction::StrToLower( header, header.length() );
	index = header.find("charset=");
	if (index != string::npos)
	{
		 pre_index = index + sizeof("charset=") -1;
		index = header.find_first_of(delims,  pre_index );
		if(index != string::npos){
			this->char_set = header.substr( pre_index, index -  pre_index);
		}
	}
}


//得到网页体编码
void CPage::GetContentEncoding(string header)
{
	string::size_type pre_index,index;
	const string delims("\r\n");
	string str = header;

	CStrFunction::StrToLower(header,header.length());
	index = header.find("content-encoding:");

	if(index != string::npos)
	{
		pre_index = index + sizeof("content-encoding: ") -1;
		index = header.find_first_of(delims,pre_index);
		if(index != string::npos)
		{
			this->content_encoding = str.substr(pre_index,index-pre_index);
		}
	}
}


//得到连接状态
void CPage::GetConnectionState(string header)
{
	string::size_type pre_index,index;
	const string delims(";\r\n");
	string str = header;

	CStrFunction::StrToLower(header,header.length());
	index = header.find("connection:");

	if(index != string::npos)
	{
		pre_index = index + sizeof("connection: ") -1;
		index = header.find_first_of(delims,pre_index);
		if(index != string::npos)
		{
			this->location = str.substr(pre_index,index-pre_index);
		}
		if (str == "keep-alive") 
			this->connection_state = true;
	}

}

//得到网页体类型
void CPage::GetContentType(string header)
{
	string::size_type pre_index,index;
	const string delims(";\r\n");
	string str = header;

	CStrFunction::StrToLower(header,header.length());
	index = header.find("content-type:");

	if(index != string::npos)
	{
		pre_index = index + sizeof("content-type: ") -1;
		index = header.find_first_of(delims,pre_index);
		if(index != string::npos)
		{
			this->content_type = str.substr(pre_index,index-pre_index);
		}
	}
}


//得到网页体的传输编码方式
void CPage::GetTransferEncoding(string header)
{
	string::size_type pre_index,index;
	const string delims(";\r\n");
	string str = header;

	CStrFunction::StrToLower(header,header.length());
	index = header.find("transfer-encoding:");

	if(index != string::npos)
	{
		pre_index = index + sizeof("transfer-encoding: ") -1;
		index = header.find_first_of(delims,pre_index);
		if(index != string::npos)
		{
			this->transfer_encoding = str.substr(pre_index,index-pre_index);
		}
	}
}


//判断一个URL是不是应该过滤，要过滤返回true否则返回false
bool CPage::IsFilterLink(string plink)
{
	
	if(plink.empty())
		return true;
	if(plink.size() > URL_LEN)
		return true;

	string link = plink;
	string tmp;

	string::size_type index = 0;
	CStrFunction::StrToLower(link,link.length());

	//find two times following symbols, return false
	tmp = link;
	index = tmp.find("?");//url中出现两个?要过滤
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("?");
		if(index != string::npos)
			return true;
	}

	//先后出现'-'和'+'字符要过滤
	tmp = link;
	index = tmp.find("-");
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("+");
		if(index != string::npos)
			return true;
	}

	//出现2个'&'字符要过滤
	tmp = link;
	index = tmp.find("&");
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("&");
		if(index != string::npos)
			return true;
	}

	//出现2个"//"字符要过滤
	tmp = link;
	index = tmp.find("//");
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("//");
		if(index != string::npos)
			return true;
	}

	//出现2个"http"要过滤
	tmp = link;
	index = tmp.find("http");
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("http");
		if(index != string::npos)
			return true;
	}

	//出现2个"misc"要过滤
	tmp = link;
	index = tmp.find("misc");
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("misc");
		if(index != string::npos)
			return true;
	}

	//出现2个"ipb"要过滤
	tmp = link;
	index = tmp.find("ipb");
	if(index != string::npos)
	{
		tmp = tmp.substr(index+1);
		index = tmp.find("ipb");
		if(index != string::npos)
			return true;
	}

	//use the robots.txt of www.expasy.org
	const char *filter_str[] = 
	{
		"cgi-bin",	"htbin",	"linder",	"srs5",		"uin-cgi", 
		"uhtbin",	"snapshot",	"=+",		"=-",		"script",
		"gate",		"search",	"clickfile",	"data/scop",	"names",
		"staff/",	"enter",	"user",		"mail",	"pst?",
		"find?",	"ccc?",		"fwd?",		"tcon?",	"&amp",
		"counter?",	"forum",	"cgisirsi",	"{",		"}",
		"proxy",	"login",	"00.pl?",	"sciserv.pl",	"sign.asp",
		"<",		">",		"review.asp?",	"result.asp?",	"keyword",
		"\"",		"'",		"php?s=",	"error",	"showdate",
		"niceprot.pl?",	"volue.asp?id",	".css",		".asp?month",	"prot.pl?",
		"msg.asp",	"register.asp", "database",	"reg.asp",	"qry?u",
		"p?msg",	"tj_all.asp?page", ".plot.",	"comment.php",	"nicezyme.pl?",
		"entr",		"compute-map?", "view-pdb?",	"list.cgi?",	"lists.cgi?",
		"details.pl?",	"aligner?",	"raw.pl?",	"interface.pl?","memcp.php?",
		"member.php?",	"post.php?",	"thread.php",	"bbs/",		"/bbs"
	};



	for(int i=0;i<75;i++)
	{
		if(link.find(filter_str[i]) != string::npos )
			return true;
	}

		cout<<"get the IsFilterlink"<<endl;
	return false;
}

/*****************************************************************
** Function name: GetContentLinkInfo
** Input argv:
**      --
** Output argv:
**      --
** Return:
        true: success
        false: fail
** Function Description:  Parse hyperlinks from the web page
*****************************************************************/
bool CPage::GetContentLinkInfo()
{
	if(body_content.empty())
		return false;

	content_link_info = body_content;
	string &s = content_link_info;

	const string delims(" \t\r\n");
	string::size_type pre_index , index = 0;

	//找到所有的"\t\r\n"并将'\t'替换为' ' 如果是\t\t\r\n则删除一个\t
	while( (index = s.find_first_of(delims,index)) != string::npos)
	{
		cout<<"index: "<<index<<endl;
		pre_index = index;
		s.replace(index,1,1,' ');
		index++;

		while((index = s.find_first_of(delims,index)) != string::npos)
		{
			if(index-pre_index == 1)
			{
				s.erase(index,1);
			}
			else
				break;
		}
		
		index--;
	}

	

		//将s中<br>标记全部替换为空格
	CStrFunction::ReplaceStr(s,"<br>"," ");

	if(s.size()<20)
		return false;

		//keep only <img ...>, <area ...>,<script ...> and <a href ...> tags.
	string::size_type index_href = 0 , index_area = 0 , index_imag = 0;
	string dest;
	do
	{
		if(s.empty())
			break;
		index_href = CStrFunction::FindCase(s,"href");
		index_area = CStrFunction::FindCase(s,"<area");
		index_imag = CStrFunction::FindCase(s,"<img");

		pre_index = index_href > index_area ? index_area : index_href;
		pre_index = index_imag > pre_index ? pre_index : index_imag;

		if(pre_index == string::npos)
			break;

		s = s.substr(pre_index);
		index = s.find_first_of('<',1);

		if( index != string::npos )
		{
			dest = dest + s.substr(0,index);
		}
		else
			break;

		s = s.substr(index);
		index_href = 0 , index_area = 0 , index_imag = 0;
	}while(1);

	s = dest;		

	//erase all '\' character
	CStrFunction::EraseStr(s,"\\");

	if(s.size() <20)
		return false;

	return true;
}


/*****************************************************************
** Function name: GetLinkInfoForPSE();
** Input argv:
**      --  
** Output argv:
**      --
** Return:
       true: success
       false: fail
** Function Description:  Get links for PSE
*****************************************************************/
bool CPage::GetLinkInfoForPSE()
{
	if(this->content_link_info.empty())
		return false;
	this->link_for_search = this->content_link_info;

	string &s = this->link_for_search;

	//keep only <area ...>,and <a href ...> tags.
	string::size_type index_href = 0,index_area = 0,
		index,pre_index;
	string dest;

	do{
		if( s.empty() ) break;


		index_href = CStrFunction::FindCase(s, "href");
		index_area = CStrFunction::FindCase(s, "<area ");

		pre_index = index_href > index_area ? index_area : index_href ;

		if( pre_index == string::npos) break;

		s = s.substr(pre_index);
		index = s.find_first_of('<',1);

		if( !(s.length() < 4) ){
			index_href = CStrFunction::FindCaseFrom(s, "href", 4);
			index = index > index_href ? index_href : index;
		}

		if( index != string::npos ){
			dest = dest + s.substr(0,index);
		}else if (index == string::npos && pre_index != string::npos){
			dest = dest + s;
			break;
		}else{
			break;
		}

		s = s.substr(index);
		index_href=0; index_area=0;
	}while(1);
		
	s = dest;
	if( s.length() < 20 ) return false;


	// erase all '"' , '\'', "&nbsp;".
	CStrFunction::EraseStr(s, "\"");
	CStrFunction::EraseStr(s, "'");
	CStrFunction::EraseStr(s, "&nbsp");

 	//keep URLs and anchor text.

	index_href = 0;
	const string delims( " #>");
	dest.clear();

	do{
		if( s.empty() ) break;
		index_href = CStrFunction::FindCase(s, "href");

		if( index_href == string::npos) break;
		pre_index = index_href;

		//####
		index = s.find('=', index_href);
		if( index == string::npos ) break;
		s = s.substr(index+1);

		while( s.length() > 0 && s[0] == ' ' ){
			s.erase(0,1);
		}
		if( s.length() == 0 ) break;

		index = s.find_first_of(delims,1);
		//cout << endl << s.substr(0, idx) << endl;
		if( index == string::npos ) break;

		dest += '"' + s.substr(0, index);

		//cout << endl << dest << endl;
			
		index = s.find('>');
		if( index == string::npos ) break;
		dest += '>';
		s = s.substr(index +1);
			
		index = s.find('<');

		if( !s.empty() ){
			index_href = CStrFunction::FindCase(s, "href");
			index = index > index_href ? index_href : index;
		}

		if( index == string::npos ){
			dest += s;
			break;
		}



		dest += s.substr(0,index);
	

		index_href = 0;
	}while(1);
		
	// look for empty filenames.
	index = 0;
	while( (index = dest.find("\"\"",index)) != string::npos ){
		dest.erase(index, 1);
	}

	s = dest;

	return( s.length() < 20 ? false : true );
}

/*****************************************************************
** Function name: GetLinkInfoForHistory()
** Input argv:
**      --  
** Output argv:
**      --
** Return:
       true: success
       false: fail
** Function Description:  Get links for history archiving
*****************************************************************/
bool CPage::GetLinkInfoForHistory()
{
	if( this->content_link_info.empty() ) 
		return false;

	this->link_for_history = this->content_link_info;
	string& s = this->link_for_history ;

 	// Keep only <img ...> tags.
	string::size_type index_imag = 0,
		index , pre_index;
	string dest;

	do{
		if( s.empty() ) break;
		index_imag = CStrFunction::FindCase(s, "<img");

		pre_index = index_imag;
		if( pre_index == string::npos) break;

		s = s.substr(pre_index);
		index = s.find_first_of('<',1);

		if( index != string::npos ){
			dest = dest + s.substr(0,index);
		}else if (index == string::npos && pre_index != string::npos){
			dest = dest + s;
			break;
		}else{
			break;
		}

		s = s.substr(index);
		index_imag = 0;
	}while(1);
		
	s = dest;
	if( s.length() < 20 ) return false;

	// erase all '"'. '\'',"&nbsp;".
	CStrFunction::EraseStr(s , "\"");
	CStrFunction::EraseStr(s , "'");
	CStrFunction::EraseStr(s , "&nbsp");

 	// Keep URLs and anchor text.

	index_imag = 0;
	string::size_type idxSrc = 0;
	const string delims( " #>");
	dest.clear();

	do{
		if( s.empty() ) break;
		index_imag = CStrFunction::FindCase(s, "<img");

		if( index_imag == string::npos) break;
		pre_index = index_imag;

		s = s.substr(index_imag+5);		// skip "img"

		//####
		index = s.find('>', index_imag);
		if( index_imag == string::npos) break;
		if( s.empty() ) break;
		idxSrc = CStrFunction::FindCase(s, "src");
		if( idxSrc > index_imag ) continue;
		s = s.substr(idxSrc);

		//index = s.find('=', index_imag);
		index = s.find('=', idxSrc);
		if( index == string::npos ) break;
		s = s.substr(index+1);

		while( s.length() > 0 && s[0] == ' ' ){
			s.erase(0,1);
		}
		if( s.length() == 0 ) break;

		index = s.find_first_of(delims,1);
		if( index == string::npos ) break;

		if( s.at(0) == '"'){
			dest += s.substr(0, index);
		}else{
			dest += '"' + s.substr(0, index);
		}
			
		index = s.find('>');
		if( index == string::npos ) break;
		dest += '>';
		s = s.substr(index +1);
			
		index = s.find('<');
		if( index == string::npos ){
			//dest += s;
			break;
		}
		//dest += s.substr(0,index);
		//####

		index_imag = 0;
	}while(1);
		

	// look for empty filenames.
	index = 0;
	while( (index = dest.find("\"\"",index)) != string::npos ){
		dest.erase(index, 1);
	}

	s = dest;

	return( s.length() < 20 ? false: true );
}
/*****************************************************************
** Function name: ParseHyperLinks
** Input argv:
**      --
** Output argv:
**      --
** Return:
        true: success
        false: fail
** Function Description:  Parse hyperlinks from the web page
*****************************************************************/
bool CPage::ParseHyperLinks()
{
	if(this->GetContentLinkInfo() == false)
		return false;

	cout<<"GetContentLinkInfo() finished"<<endl;

	if(this->content_link_info.empty())
		return false;
	
	bool find_for_pse = false;
	bool find_for_history = false;

	if(this->GetLinkInfoForPSE())
		if(this->FindRefLinkForPSE())
			find_for_pse = true;

	cout<<"FindRefLinkForPSE() finished"<<endl;

	if(this->GetLinkInfoForHistory())
		if(this->FindRefLinkForHistory())
			find_for_pse = true;

	cout<<"FindRefLinkForHistory() finished"<<endl;

	if(!find_for_pse && !find_for_history)
		return false;

	return true;
}

//判断strUrl是不是正规的url
bool CPage::NormalizeUrl(string &str_url)
{
	string::size_type index;

	//URL没有htp://协议名我们这里认为strUrl不是正规的URL
	if(CStrFunction::FindCase(str_url,"http://") == string::npos)
		return false;

	//convert "http://**.**.**"to "http://**.**.**/"
	index = str_url.rfind('/');
	if(index<8)
	{
		str_url = str_url+"/";
		return true;
	}

	//将"/./"转换成"/"
	while(index = str_url.find("/./") != string::npos)
	{
		if(index != string::npos)
			str_url.erase(index,2);
	}

	//将"xxx/../yyy"转换成"xxx/yyy"
	while(index = str_url.find("/../") != string::npos)
	{
		string str_pre , str_after;

		str_pre = str_url.substr(0,index);
		if(str_url.length()>index+4)
		{
			str_after = str_url.substr(index+4);
		}

		index = str_pre.rfind("/");
		if(index != string::npos)
			str_pre = str_pre.substr(0,index+1);
		if(str_pre.length()<10)
			return false;

		str_url = str_pre+str_after;
	}

	if(CStrFunction::FindCase(str_url,"http://") != 0)
		return false;

	return true;
}

/*最终得到为搜索引擎准备的超链接并将相对路径的URL和绝对路径的URL分别处理，
同时，我们发现从一个网页中提取的超链接可以是相同的，这个时候我们必须去重，
这个函数用map容器很好的做到了这一点还有一些URL不是正规的URL也要过滤 还有一
些URL是必要过滤也要过滤--通过IsFilterLink(string strUrl)实现*/
bool CPage::FindRefLinkForPSE()
{
	if(this->link_for_search.empty())
		return false;

	char *buffer = (char *)this->link_for_search.c_str();
	int url_num = 0;
	int len;
	char *ptr;

	static char buf[URL_REFERENCES_LEN];

	memset(buf,0,URL_REFERENCES_LEN);

	len = strlen(buffer);
	if(len<8)
		return false;

	len = len < URL_REFERENCES_LEN -1 ? len : URL_REFERENCES_LEN - 1;//len记录相对较小的值
	strncpy( buf, buffer, len);

	//分别提取出url和其描述符
	ptr = buf;
	while( ptr - buf < len  && *ptr ){
		while( *ptr == '"' && *ptr) ptr++;
		if ( !*ptr ) break;
		this->page_link[ url_num].link = ptr;
		while( *ptr && *ptr != '>'){
			if(*ptr == ' ') *ptr = '\0';
			ptr++;
		}

		if ( !*ptr ){
			url_num++;
			break;
		}
		if ( *ptr == '>' ){
			*ptr++='\0';
			if( !*ptr ){
				url_num++;
				break;
			}
			if( *ptr == '"' ){
				this->page_link[ url_num].anchor_text = NULL;
			}else{
				this->page_link[ url_num].anchor_text = ptr;
				while( *ptr && *ptr != '"') ptr++;
				if (!*ptr){
					url_num++;
					break;
				}
				if ( *ptr == '"') *ptr='\0';
			}

		}
		
		ptr++;
		url_num++;
		if ( url_num == MAX_URL_REFERENCES) break;
	}
	

	this->page_link_number = url_num;


	typedef map<string,string>::value_type valType;

	map_link_for_pse.clear();

	//string strRootUrl= m_sUrl;
	CUrl iUrl;
	if( iUrl.ParseUrl(this->url) == false ){
		cout << "ParseUrlEx error in FindRefLinkForPSE(): " <<this->url << endl;
		return false;
	}
	
	for(int i=0; i<this->page_link_number; i++){

		string str;
		string::size_type idx;
		const string delims(" #");

		str = page_link[i].link;
		idx = str.find_first_of(delims, 0 );
		if( idx != string::npos ){
			str = str.substr(0, idx);
		}
		if( str.size() == 0 || str.size() > URL_LEN - 1 
			|| str.size() < 4 ) continue;


		string::size_type idx1;
		idx1 = CStrFunction::FindCase(str, "http");
		if( idx1 != 0  ){
			char c1 = this->url.at(this->url.length()-1);
			char c2 = str.at(0);

			if( c2=='/' ){
				if( iUrl.port_number != 80 ){
					cout << iUrl.host_name << endl;
					cout << str << endl;

					str = "http://" + iUrl.host_name + ":" + CStrFunction::itos(iUrl.port_number) + str;
				} else {
					str = "http://" + iUrl.host_name + str;
				}
			} else if( c1!='/' && c2!='/'){
				string::size_type idx;

				idx = this->url.rfind('/');
				if( idx != string::npos ){
					if( idx > 6 ){ // > strlen("http://..")
						str = this->url.substr(0, idx+1) + str;
					} else {
						str = this->url + "/" + str;
					}

				} else {

					continue;
				}

			} else {
				if( c1=='/' ){
					str = this->url + str;
				} else {
					str = this->url + "/" + str;
				}
			}
		}

		if( NormalizeUrl(str) == false ) continue;

		if( IsFilterLink(str) ) continue;

		//debug
		//cout << "reflink: " << reflink << endl;

		if( str == this->url ){
			continue;
		}else{
			if( page_link[i].anchor_text ){
				if( map_link_for_pse.find(str) == map_link_for_pse.end() ){
					map_link_for_pse.insert( valType( str, page_link[i].anchor_text));
				}
			}else{
				if( map_link_for_pse.find(str) == map_link_for_pse.end() ){
					map_link_for_pse.insert( valType( str, "\0") );
					cout << ".";
				}
			}
		}
			

	}

	page_link_number = map_link_for_pse.size();


	return true;
}

/*最终得到为历史网页存档准备的超链接并将相对路径的URL和绝对路径的URL分别处理，
同时，我们发现从一个网页中提取的超链接可以是相同的，这个时候我们必须去重，这个
函数用vector容器很好的做到了这一点还有一些URL不是正规的URL也要过滤还有一些URL
是必要过滤也要过滤--通过IsFilterLink(string strUrl)实现*/
bool CPage::FindRefLinkForHistory()
{
	if(this->link_for_history.empty())
		return false;

	char *buffer = (char *)this->link_for_history.c_str();
	int url_num = 0;
	int len;
	char *ptr;

	static char buf[URL_REFERENCES_LEN/2];

	memset(buf,0,URL_REFERENCES_LEN/2);

	len = strlen(buffer);
	if(len<8)
		return false;

	len = len < URL_REFERENCES_LEN/2 -1 ? len : URL_REFERENCES_LEN/2 - 1;//len记录相对较小的值
	strncpy( buf, buffer, len);

	//分别提取出url和其描述符
	ptr = buf;
	while( ptr - buf < len  && *ptr ){
		while( *ptr == '"' && *ptr) ptr++;
		if ( !*ptr ) break;
		this->history_page_link[ url_num].link = ptr;
		while( *ptr && *ptr != '>'){
			if(*ptr == ' ') *ptr = '\0';
			ptr++;
		}

		if ( !*ptr ){
			url_num++;
			break;
		}
		if ( *ptr == '>' ){
			*ptr++='\0';
			if( !*ptr ){
				url_num++;
				break;
			}
			if( *ptr == '"' ){
				//this->history_page_link[ url_num].link = NULL;
			}else{

				while( *ptr && *ptr != '"') ptr++;
				if (!*ptr){
					url_num++;
					break;
				}
				if ( *ptr == '"') *ptr='\0';
			}

		}
		
		ptr++;
		url_num++;
		if ( url_num == MAX_URL_REFERENCES/2) break;
	}
	

	this->history_page_link_number = url_num;


	typedef map<string,string>::value_type valType;

	vector_link_for_history.clear();

;
	CUrl iUrl;
	if( iUrl.ParseUrl(this->url) == false ){
		cout << "ParseUrlEx error in FindRefLinkForHistory(): " <<this->url << endl;
		return false;
	}
	
	for(int i=0; i<this->history_page_link_number; i++){

		string str;
		string::size_type idx;
		const string delims(" #");

		str = history_page_link[i].link;
		idx = str.find_first_of(delims, 0 );
		if( idx != string::npos ){
			str = str.substr(0, idx);
		}
		if( str.size() == 0 || str.size() > URL_LEN - 1 
			|| str.size() < 4 ) continue;


		string::size_type idx1;
		idx1 = CStrFunction::FindCase(str, "http");
		if( idx1 != 0  ){
			char c1 = this->url.at(this->url.length()-1);
			char c2 = str.at(0);

			if( c2=='/' ){
				if( iUrl.port_number != 80 ){
					cout << iUrl.host_name << endl;
					cout << str << endl;

					str = "http://" + iUrl.host_name + ":" + CStrFunction::itos(iUrl.port_number) + str;
				} else {
					str = "http://" + iUrl.host_name + str;
				}
			} else if( c1!='/' && c2!='/'){
				string::size_type idx;

				idx = this->url.rfind('/');
				if( idx != string::npos ){
					if( idx > 6 ){ // > strlen("http://..")
						str = this->url.substr(0, idx+1) + str;
					} else {
						str = this->url + "/" + str;
					}

				} else {

					continue;
				}

			} else {
				if( c1=='/' ){
					str = this->url + str;
				} else {
					str = this->url + "/" + str;
				}
			}
		}

		if( NormalizeUrl(str) == false ) continue;

		if( IsFilterLink(str) ) continue;

	

		if( str == this->url )
		{
			continue;
		}else
		{
			vector<string>::iterator it;
            it = find(vector_link_for_history.begin(), vector_link_for_history.end(),str);
			if( it == vector_link_for_history.end() )
			{
				vector_link_for_history.push_back( str);
				cout << ".";
            }
		}

   }
    history_page_link_number = vector_link_for_history.size();

	page_link_number = map_link_for_pse.size();


	return true;
}
