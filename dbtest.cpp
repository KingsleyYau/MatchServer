/*
 * dbtest.cpp
 *
 *  Created on: 2015-1-14
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>

#include <string>
using namespace std;

#include "DBManagerTest.h"

int iMemory = 1;	// sqlite句柄数目, 即内存数据库数目
int iThread = 1;	// 线程数目
int iTask = 1;		// 总查询次数
string sql = "";

bool Parse(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	printf("############## dbtest ############## \n");
	Parse(argc, argv);

	DBManagerTest test;
	if( sql.length() == 0 ) {
		test.StartTest(iThread, iMemory, iTask);
	} else {
		test.TestSql(sql);
	}

	return EXIT_SUCCESS;
}

bool Parse(int argc, char *argv[]) {
	string key;
	string value;

	for( int i = 1; (i + 1) < argc; i+=2 ) {
		key = argv[i];
		value = argv[i+1];

		if( key.compare("-t") == 0 ) {
			iThread = atoi(value.c_str());
		} else if( key.compare("-n") == 0 ) {
			iTask = atoi(value.c_str());
		} else if( key.compare("-m") == 0 ) {
			iMemory = atoi(value.c_str());
		} else if( key.compare("-sql") == 0 ) {
			sql = value;
		}
	}

	printf("# iThread : %d, iTask : %d, iMemory : %d\n", iThread, iTask, iMemory);

	return true;
}
