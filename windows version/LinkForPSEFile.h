#ifndef _LINKFORPSEFILE_H
#define _LINKFORPSEFILE_H

#include "FileName.h"
#include "Url.h"
#include "Page.h"
#include "FileEngine.h"

class CLinkForPSEFile:public CFileEngine
{
public:
	CLinkForPSEFile();
	CLinkForPSEFile(string str);
	virtual ~CLinkForPSEFile();

	inline int GetFileType(){return LINKFORPSE;}

	virtual bool Write(void *arg);
};

#endif