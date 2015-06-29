#ifndef _STR_FUNCTION_H
#define _STR_FUNCTION_H

#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

using namespace std;

class CStrFunction
{
public:
	CStrFunction();
	virtual ~CStrFunction();
	 

	//change the capital into low case
	static void StrToLower(string &str , int length);
	//int to string
	static string itos(long long i )
	{
		stringstream s;
		s << i;
		return s.str();
	}

	//find the second from first , return the pos
	static string::size_type FindCase(string hay_stack , string needle);
	static string::size_type FindCaseFrom(string hay_stack , string needle , int from);

	//
	static void ReplaceStr(string &str, string srstr, string dsstr);
	static void EraseStr(string &str, string sub_str);

};

#endif