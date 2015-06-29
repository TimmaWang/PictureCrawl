#ifndef _PSEFILE_H
#define _PSEFILE_H
using namespace std;

#include "FileName.h"
#include "Page.h"
#include "Url.h"
#include "FileEngine.h"

class CPSEFile:public CFileEngine
{
public:
	CPSEFile();
	CPSEFile(string str);
	virtual ~CPSEFile();

	inline int GetFileType(){return PSE;}
	virtual bool Write(void *org);




};

#endif