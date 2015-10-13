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
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <string>
using namespace std;

#include "TcpTestClient.h"

char ip[128] = {'\0'};
int iPort = 1;
int iTotal = 1;

bool Connnect(int i);
bool Parse(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	printf("############## client ############## \n");
	Parse(argc, argv);

	int i = 0;
	bool bFlag = true;
	while( i < iTotal && bFlag ) {
		bFlag = Connnect(i++);
	}

	while( true ) {
		sleep(3);
	}

	return EXIT_SUCCESS;
}

bool Parse(int argc, char *argv[]) {
	string key;
	string value;

	for( int i = 1; (i + 1) < argc; i+=2 ) {

		key = argv[i];
		value = argv[i+1];

		if( key.compare("-h") == 0 ) {
			memcpy(ip, value.c_str(), value.length());
		} else if( key.compare("-p") == 0 ) {
			iPort = atoi(value.c_str());
		} else if( key.compare("-n") == 0 ) {
			iTotal = atoi(value.c_str());
		}
	}

	printf("# ip : %s, iPort : %d\n", ip, iPort);

	return true;
}

bool Connnect(int i) {
	struct sockaddr_in mAddr;
	bzero(&mAddr, sizeof(mAddr));
	mAddr.sin_family = AF_INET;
	mAddr.sin_port = htons(iPort);
	mAddr.sin_addr.s_addr = inet_addr(ip);

	char buffer[2048] = {'\0'};
	int iFlag = 1;
	int mClient;
	if ((mClient = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("# buffer( Create socket error ) \n");
		return false;
	}

	setsockopt(mClient, IPPROTO_TCP, TCP_NODELAY, (const char *)&iFlag, sizeof(iFlag));
	setsockopt(mClient, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));

	if( connect(mClient, (struct sockaddr *)&mAddr, sizeof(mAddr)) != -1 ) {
//		LogManager::GetLogManager()->Log("TcpTestClient::Connnect( connect ok fd : %d )", mClient);
		printf("# Connnect( connect ok fd : %d ) \n", mClient);
		char msg[1024] = {'\0'};
		sprintf(msg, "client : %d", i);
		char sendBuffer[2048] = {'\0'};
		memset(sendBuffer, '\0', 2048);
		snprintf(sendBuffer, 2048 - 1, "POST %s HTTP/1.0\r\nContent-Length: %d\r\nConection: %s\r\n\r\n%s",
									"/QUERY?MANID=CM1000&SITEID=1",
									strlen(msg),
									"Keep-alive",
									msg
									);
		send(mClient, sendBuffer, strlen(sendBuffer), 0);
		printf("# Connnect( send ok fd : %d, client : %d ) \n", mClient, i);
//
//		while( 1 ) {
//			/* recv from call server */
//			int ret = recv(mClient, buffer, 2048 - 1, 0);
//			if( ret > 0 ) {
//				printf("# Connnect( recv message ok ) \n");
//			} else {
//				printf("# Connnect( recv fail disconnnect ret : %d ) \n", ret);
//				break;
//			}
//		}
	} else {
		printf("# Connnect( connnect fail ) \n");
		return false;
	}

//	close(mClient);
	printf("# Connnect( exit ) \n");
	return true;
}
