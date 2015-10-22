/*
 * DataHttpParser.h
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#ifndef DATAHTTPPARSER_H_
#define DATAHTTPPARSER_H_

#include "DataParser.h"
#include "Arithmetic.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <algorithm>

#include <string>
#include <map>
using namespace std;

typedef map<string, string> Parameters;

typedef enum HttpType {
	GET,
	POST,
	UNKNOW,
};

class DataHttpParser : public DataParser {
public:
	DataHttpParser();
	virtual ~DataHttpParser();

	int ParseData(char* buffer);

	const char* GetParam(const char* key);
	const char* GetPath();
	HttpType GetType();

private:
	HttpType mHttpType;
	int miContentLength;
	Parameters mParameters;
	string mPath;

	void ParseFirstLine(char* buffer);
	void ParseParameters(char* buffer);
};

#endif /* DATAHTTPPARSER_H_ */
