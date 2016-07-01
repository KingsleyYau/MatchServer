/*
 * DataHttpParser.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#include "DataHttpParser.h"

#define MAX_URL 2048
#define MAX_FIRST_LINE 4096

DataHttpParser::DataHttpParser() {
	// TODO Auto-generated constructor stub
	Reset();
}

DataHttpParser::~DataHttpParser() {
	// TODO Auto-generated destructor stub
}

void DataHttpParser::Reset() {
	miContentLength = -1;
	mHttpType = UNKNOW;
	mPath = "";
	mParameters.clear();
}

int DataHttpParser::ParseData(char* buffer, int len) {
	int result = 0;

	// 解析http头部first line
	string headers = buffer;

	string::size_type pos = headers.find("\r\n");
	if( pos != string::npos ) {
		// Read first line
		string firstLine = headers.substr(0, pos);
		result = pos + 2;

		// 只解析第一行
		if( ParseFirstLine(firstLine) ) {
			// success

		} else {
			// fail
			result = -1;
		}

	} else if( headers.length() > MAX_FIRST_LINE || len > MAX_FIRST_LINE ) {
		// 第一行超过4096 bytes
		result = -1;

	}

	return result;
}

string DataHttpParser::GetParam(const string& key)  {
	string result = "";
	Parameters::iterator itr = mParameters.find(key.c_str());
	if( itr != mParameters.end() ) {
		result = (itr->second);
	}
	return result;
}

string DataHttpParser::GetPath() {
	return mPath;
}

HttpType DataHttpParser::GetType() {
	return mHttpType;
}

bool DataHttpParser::ParseFirstLine(const string& wholeLine) {
	bool bFlag = true;
	string line;
	int i = 0;
	string::size_type index = 0;
	string::size_type pos;

	while( string::npos != index ) {
		pos = wholeLine.find(" ", index);
		if( string::npos != pos ) {
			// 找到分隔符
			line = wholeLine.substr(index, pos - index);
			// 移动下标
			index = pos + 1;

		} else {
			// 是否最后一次
			if( index < wholeLine.length() ) {
				line = wholeLine.substr(index, pos - index);
				// 移动下标
				index = string::npos;

			} else {
				// 找不到
				index = string::npos;
				break;
			}
		}

		switch(i) {
		case 0:{
			// 解析http type
			if( strcasecmp("GET", line.c_str()) == 0 ) {
				mHttpType = GET;

			} else if( strcasecmp("POST", line.c_str()) == 0 ) {
				mHttpType = POST;

			} else {
				bFlag = false;
				break;
			}
		}break;
		case 1:{
			// 解析url
			Arithmetic ari;
			char temp[MAX_URL] = {'\0'};
			int decodeLen = ari.decode_url(line.c_str(), line.length(), temp);
			temp[decodeLen] = '\0';
			string path = temp;
			string::size_type posSep = path.find("?");
			if( (string::npos != posSep) && (posSep + 1 < path.length()) ) {
				// 解析路径
				mPath = path.substr(0, posSep);

				// 解析参数
				string param = path.substr(posSep + 1, path.length() - (posSep + 1));
				ParseParameters(param);

			} else {
				mPath = path;
			}

		}break;
		default:break;
		};

		i++;
	}

	return bFlag;
}

void DataHttpParser::ParseParameters(const string& wholeLine) {
	string key;
	string value;

	string line;
	string::size_type posSep;
	string::size_type index = 0;
	string::size_type pos;

	while( string::npos != index ) {
		pos = wholeLine.find("&", index);
		if( string::npos != pos ) {
			// 找到分隔符
			line = wholeLine.substr(index, pos - index);
			// 移动下标
			index = pos + 1;

		} else {
			// 是否最后一次
			if( index < wholeLine.length() ) {
				line = wholeLine.substr(index, pos - index);
				// 移动下标
				index = string::npos;

			} else {
				// 找不到
				index = string::npos;
				break;
			}
		}

		posSep = line.find("=");
		if( (string::npos != posSep) && (posSep + 1 < line.length()) ) {
			key = line.substr(0, posSep);
			value = line.substr(posSep + 1, line.length() - (posSep + 1));
			mParameters.insert(Parameters::value_type(key, value));

		} else {
			key = line;
		}

	}
}
