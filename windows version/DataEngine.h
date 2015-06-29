#ifndef _DATAENGINE_H
#define _DATAENGINE_H

#include "FileName.h"

enum data_engine_type
{
	FILE_ENGINE,
	DATABASE_ENGINE
};

class CDataEngine
{
public:
	string str;

public:
	CDataEngine(string str);
	CDataEngine();
	~CDataEngine();

	virtual int GetEngineType() = 0;
	virtual bool Write(void *arg) = 0;
	virtual bool Open(string str) = 0;
	virtual void Close() = 0;

};
#endif