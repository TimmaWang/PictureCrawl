#ifndef _MD5_H
#define _MD5_H

#include <string>
#include <iostream>
using namespace std;

class CMD5
{
private:
	#define uint8  unsigned char
	#define uint32 unsigned long int

	struct md5_context
	{
		uint32 total[2];
		uint32 state[4];
		uint8  buffer[64];

	};

	void Md5Init(struct md5_context *context);
	void Md5Process(struct md5_context *context,uint8 data[64]);
	void Md5Update(struct md5_context *context,uint8 *input,uint32 length);
	void Md5Finished(struct md5_context *context,uint8 degist[16]);

public:

	//construct a CMD5 from any buffer
	void GenerateMd5(unsigned char *buffer,int buffer_length);
	CMD5(); 
	//~CMD5();

	//construct a CMD5
	CMD5(const char * md5_source);

	//construct a CMD5 from a 16 bytes md5
	CMD5(unsigned long* md5_source);

	//add a other md5
	CMD5 operator +(CMD5 adder);

	//just if equal
	bool operator ==(CMD5 cmper);

	//give the value from equer
	//void operator =(CMD5 equer);
	//to a string
	string ToString();

	unsigned long data[4];
};

#endif