/*
 * server.cpp
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

#include <string>
using namespace std;

#include "MatchServer.h"
#include "LogManager.h"

int iMemory = 1;	// sqlite句柄数目, 即内存数据库数目
int iThread = 1;	// 线程数目

// Just 4 log
#define MaxMsgDataCount 200000

bool Parse(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	printf("############## Match Server ############## \n");

	Parse(argc, argv);

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

	/* log system */
	LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
//	LogManager::LogSetFlushBuffer(0);
	LogManager::GetLogManager()->Start(MaxMsgDataCount, LOG_STAT);
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::Run( MaxMsgDataCount : %d )", MaxMsgDataCount);

	MatchServer server;
	server.Run(100000, iMemory, iThread);

	return EXIT_SUCCESS;
}

bool Parse(int argc, char *argv[]) {
	string key, value;
	for( int i = 1; (i + 1) < argc; i+=2 ) {
		key = argv[i];
		value = argv[i+1];

		if( key.compare("-t") == 0 ) {
			iThread = atoi(value.c_str());
		} else if(  key.compare("-m") == 0 ) {
			iMemory = atoi(value.c_str());
		}
	}

	printf("# iThread : %d, iMemory : %d\n", iThread, iMemory);

	return true;
}
