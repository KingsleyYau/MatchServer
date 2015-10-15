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


string sConf = "";  // 配置文件

bool Parse(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	printf("############## Match Server ############## \n");
	printf("# Version : 1.0 \n");
#ifdef BUILD_DATE
	if( strlen(BUILD_DATE) > 0 )
		printf("# Build date : %s \n", BUILD_DATE);
#endif

	srand(time(0));

	/* Ignore SIGPIPE */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

	Parse(argc, argv);

	MatchServer server;
	if( sConf.length() > 0 ) {
		server.Run(sConf);
	} else {
		printf("# Usage : ./matchserver [ -f <config file> ] \n");
		server.Run("/etc/matchserver.config");
	}

	return EXIT_SUCCESS;
}

bool Parse(int argc, char *argv[]) {
	string key, value;
	for( int i = 1; (i + 1) < argc; i+=2 ) {
		key = argv[i];
		value = argv[i+1];

		if( key.compare("-f") == 0 ) {
			sConf = value;
		}
	}

//	printf("# config : %s \n", sConf.c_str());

	return true;
}
