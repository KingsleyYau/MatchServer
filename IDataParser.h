/*
 * IDataParser.h
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#ifndef IDATAPARSER_H_
#define IDATAPARSER_H_

class IDataParser {
public:
	/*
	 *	return : -1:error/0:continue/1:ok
	 */
	virtual int ParseData(char* buffer, int len) = 0;
	virtual ~IDataParser(){};
};

#endif /* IDATAPARSER_H_ */
