/*
 * DataParser.h
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#ifndef DATAPARSER_H_
#define DATAPARSER_H_

#include "IDataParser.h"
#include <string.h>

class DataParser : public IDataParser {
public:
	DataParser();
	virtual ~DataParser();

	void SetParseData(IDataParser *parser);
	virtual int ParseData(char* buffer, int len);

private:
	IDataParser *mParser;
};

#endif /* DATAPARSER_H_ */
