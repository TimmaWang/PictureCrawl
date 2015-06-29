#include "StrFunction.h"

CStrFunction::CStrFunction()
{
}

CStrFunction::~CStrFunction()
{
}

void CStrFunction::StrToLower(string &str , int length)
{
	char distance = 'A'-'a';
	for(int i=0;i<length;i++)
	{
		if(str[i]>= 'A' &&str[i]<='Z')
		{
			str[i]=str[i]-distance;
		}
	}
}

bool no_case_compare(char c1 ,char c2)
{
	return toupper(c1) == toupper(c2);
}

string::size_type CStrFunction::FindCase(string hay_stack , string needle)
{
	if(hay_stack.empty())
		return string::npos;
	if(needle.empty())
		return string::npos;

	string::iterator pos;
	pos=search(hay_stack.begin(),hay_stack.end(),needle.begin(),needle.end(),no_case_compare);

	if(pos == hay_stack.end())
		return string::npos;
	else
		return (pos-hay_stack.begin());

}

string::size_type CStrFunction::FindCaseFrom(string hay_stack , string needle , int from)
{
	if(hay_stack.empty())
		return string::npos;
	if(needle.empty())
		return string::npos;

	string::iterator pos;
	pos=search(hay_stack.begin()+from,hay_stack.end(),needle.begin(),needle.end(),no_case_compare);

	if(pos == hay_stack.end())
		return string::npos;
	else
		return (pos-hay_stack.begin());
}

void CStrFunction::ReplaceStr(string &str, string srstr, string dsstr)
{
	if(str.size()==0 || srstr.size()==0)
		return ;

	string ::size_type index = 0;
	string ::size_type sub_length = srstr.length();

	index = str.find(srstr,index);
	while(index != string::npos)
	{
		str.replace(index,sub_length,dsstr);
		if((index+dsstr.size())>str.size())
			break;

		index = str.find(srstr,index+dsstr.size());
	}
}

void CStrFunction::EraseStr(string &str, string sub_str)
{
	if(str.size() == 0||sub_str.size() == 0)
		return ;
	
	string ::size_type index = 0;
	string ::size_type sub_length = sub_str.length();

	index = str.find(sub_str,index);
	while(index != string::npos)
	{
		str.erase(index,sub_length);
		index = str.find(sub_str,index);
	}
}