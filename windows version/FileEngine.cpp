#include "FileEngine.h"

using namespace std;

CFileEngine::CFileEngine()
{

}

CFileEngine::CFileEngine(string str):CDataEngine(str)
{
	ofs_file.open(this->str.c_str(),ios::out|ios::app|ios::binary);

	if(!ofs_file)
	{
		cerr<<"can not open"<<this->str<<endl;
	}
}

CFileEngine::~CFileEngine()
{
	this->ofs_file.close();
}


bool CFileEngine::Open(string str)
{
	this->str = str;

	ofs_file.open(this->str.c_str(),ios::out|ios::app|ios::binary);

	if(!ofs_file)
	{
		cerr<<"can not open"<<this->str<<endl;
		return false;
	}
	else
		return true;
}