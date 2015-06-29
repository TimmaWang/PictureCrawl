#ifndef _ISAMFILE_H
#define _ISAMFILE_H

#include "FileEngine.h"

class CISAMFile:public CFileEngine
{
public:
	string index_file_name;
	FILE *data_file_ptr;
	FILE *index_file_ptr;

public:
	CISAMFile();
	CISAMFile(string str);
	CISAMFile(string data_str,string index_str);
	~CISAMFile();

	int GetFileType(){return ISAM;}
	virtual bool Write(void *arg);
	bool Open(string data_str,string index_str);
	virtual void Close();

	bool Open(string str){return false;}

};
#endif