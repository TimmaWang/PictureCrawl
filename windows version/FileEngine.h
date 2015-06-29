#ifndef _FILEENGINE_H
#define _FILEENGINE_H

#include <iostream>
#include <fstream>

#include "DataEngine.h"
#include "Url.h"
#include "Page.h"

enum file_engine_type
{
	ISAM,
	PSE,
	LINKFORPSE
};

struct file_arg
{
	CUrl *url_ptr;
	CPage *page_ptr;
};

class CFileEngine :public CDataEngine
{
public:
	ofstream ofs_file;

public:
	CFileEngine();
	CFileEngine(string str);
	virtual ~CFileEngine();

	int GetEngineType()
	{
		return FILE_ENGINE;
	}

	virtual int GetFileType() = 0;

	bool Open(string str);

	inline void Close()
	{
		ofs_file.close();
	}
};
#endif