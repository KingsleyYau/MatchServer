/*
 * DataHttpParser.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#include "DataHttpParser.h"

DataHttpParser::DataHttpParser() {
	// TODO Auto-generated constructor stub
	miContentLength = -1;
	mHttpType = UNKNOW;
}

DataHttpParser::~DataHttpParser() {
	// TODO Auto-generated destructor stub
}

int DataHttpParser::ParseData(char* buffer) {
	char temp[16] = {'\0'};
	int result = 1;
	int j = 0;
	char* pParam = NULL;

//	// find Body
//	char *pBody = strstr(buffer, "\r\n\r\n");
//	if( pBody != NULL ) {
//		pBody += strlen("\r\n\r\n");
//	}

	// parse header
	char *pFirst = NULL;
	char*p = strtok_r(buffer, "\r\n", &pFirst);
	while( p != NULL ) {
		if( j == 0 ) {
			ParseFirstLine(p);
			// only first line is useful
			break;
		} else {
			if( (pParam = strstr(p, "Content-Length:")) != NULL ) {
				int len = strlen("Content-Length:");
				// find Content-Length
				pParam += len;
				if( pParam != NULL  ) {
					memcpy(temp, pParam, strlen(p) - len);
					miContentLength = atoi(temp);
				}
			}
		}
		j++;
		p = strtok_r(NULL, "\r\n", &pFirst);
	}

//	if( pBody != NULL ) {
//		if( pBody != NULL && miContentLength > 0 ) {
//			if( pBody + miContentLength != NULL ) {
//				*(pBody + miContentLength) = '\0';
//				result = DataParser::ParseData(pBody);
//			}
//		}
//	}

	return result;
}

const char* DataHttpParser::GetParam(const char* key) {
	const char* result = NULL;
	Parameters::iterator itr = mParameters.find(key);
	if( itr != mParameters.end() ) {
		result = (itr->second).c_str();
	}
	return result;
}

const char* DataHttpParser::GetPath() {
	return mPath.c_str();
}

HttpType DataHttpParser::GetType() {
	return mHttpType;
}

void DataHttpParser::ParseFirstLine(char* buffer) {
	char temp[1024];
	char* p = NULL;
	int j = 0;

	char *pFirst = NULL;

	p = strtok_r(buffer, " ", &pFirst);
	while( p != NULL ) {
		switch(j) {
		case 0:{
			// type
			if( strcmp("GET", p) == 0 ) {
				mHttpType = GET;
			} else if( strcmp("POST", p) == 0 ) {
				mHttpType = POST;
			}
		}break;
		case 1:{
			// path and parameters
			char path[1025] = {'\0'};
			Arithmetic ari;
			int len = strlen(p);
			len = ari.decode_url(p, len, temp);
			char* pPath = strstr(temp, "?");
			if( pPath != NULL && ((pPath + 1) != NULL) ) {
				len = pPath - temp;
				memcpy(path, temp, len);
				ParseParameters(pPath + 1);
			} else {
				len = strlen(temp);
				memcpy(path, temp, len);
			}
			path[len] = '\0';
			mPath = path;
			transform(mPath.begin(), mPath.end(), mPath.begin(), ::toupper);
		}break;
		default:break;
		};

		j++;
		p = strtok_r(NULL, " ", &pFirst);
	}
}

void DataHttpParser::ParseParameters(char* buffer) {
	char* p = NULL;
	char* param = NULL;
	string key;
	string value;
	int j = 0;

	char *pFirst = NULL;
	char *pSecond = NULL;

	param = strtok_r(buffer, "&", &pFirst);
	while( param != NULL ) {
		j = 0;
		p = strtok_r(param, "=", &pSecond);
		while( p != NULL ) {
			switch(j) {
			case 0:{
				// key
				key = p;
				transform(key.begin(), key.end(), key.begin(), ::toupper);
			}break;
			case 1:{
				// value
				value = p;
				transform(value.begin(), value.end(), value.begin(), ::toupper);
				mParameters.insert(Parameters::value_type(key, value));
			}break;
			default:break;
			};

			j++;
			p = strtok_r(NULL, "=", &pSecond);
		}
		param = strtok_r(NULL, "&", &pFirst);
	}
}
