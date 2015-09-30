/*
 * TcpTestClient.h
 *
 *  Created on: 2015-1-14
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef CALLSERVERCLIENT_H_
#define CALLSERVERCLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "KThread.h"
#include "MessageList.h"
//#include "LogManager.h"

class TcpTestClient;
class TcpTestClientObserver {
public:
	virtual void OnRecvTcpTestClientMessage(TcpTestClient *cs, Message *m) = 0;
};
class TcpTestClient {
public:
	TcpTestClient();
	virtual ~TcpTestClient();

	bool Start(const char *ip, int port);
	bool Stop();
	bool IsRunning();
	void SetTcpTestClientObserver(TcpTestClientObserver *observer);

	void Connnect();

private:
	KThread *mpMainThread;

	bool mIsRunning;

	int mClient;
	struct sockaddr_in mAddr;

	Message mMessage;

	TcpTestClientObserver *mpTcpTestClientObserver;
};

#endif /* CALLSERVERCLIENT_H_ */
