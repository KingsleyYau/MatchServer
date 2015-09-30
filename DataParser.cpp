/*
 * DataParser.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#include "DataParser.h"

DataParser::DataParser() {
	// TODO Auto-generated constructor stub
	mParser = NULL;
}

DataParser::~DataParser() {
	// TODO Auto-generated destructor stub
}

void DataParser::SetParseData(IDataParser *parser) {
	mParser = parser;
}

int DataParser::ParseData(char* buffer) {
	if( mParser != NULL ) {
		return mParser->ParseData(buffer);
	}
	return 1;
}
