/*
 * TcpTestClient.cpp
 *
 *  Created on: 2015-1-14
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "TcpTestClient.h"

/* connnect thread */
class ConnectRunnable : public KRunnable {
public:
	ConnectRunnable(TcpTestClient *container) {
		mContainer = container;
	}
	virtual ~ConnectRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		/* run main loop */
		while( mContainer->IsRunning() ) {
			mContainer->Connnect();
			sleep(3);
		}
	}
private:
	TcpTestClient *mContainer;
};

TcpTestClient::TcpTestClient() {
	// TODO Auto-generated constructor stub
	mpTcpTestClientObserver = NULL;

	mpMainThread = new KThread(new ConnectRunnable(this));
}

TcpTestClient::~TcpTestClient() {
	// TODO Auto-generated destructor stub
	mpTcpTestClientObserver = NULL;

	Stop();
	if( mpMainThread ) {
		delete mpMainThread;
	}
}

bool TcpTestClient::Start(const char *ip, int port) {
	if( IsRunning() ) {
		return false;
	}

	mIsRunning = true;

	bzero(&mAddr, sizeof(mAddr));
	mAddr.sin_family = AF_INET;
	mAddr.sin_port = htons(port);
	mAddr.sin_addr.s_addr = inet_addr(ip);

	/* start connnect thread */
	if( mpMainThread->start() > 0 ) {
		printf("# Create connnect thread ok \n");
	}

	return true;
}

bool TcpTestClient::Stop() {
	/* stop log thread */
	mIsRunning = false;

	close(mClient);
	if( mpMainThread ) {
		mpMainThread->stop();
	}

	return true;
}

bool TcpTestClient::IsRunning() {
	return mIsRunning;
}

void TcpTestClient::SetTcpTestClientObserver(TcpTestClientObserver *observer) {
	mpTcpTestClientObserver = observer;
}

void TcpTestClient::Connnect() {
	int iFlag = 1;

	if ((mClient = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("# TcpTestClient::Start( Create socket error ) \n");
		return;
	}

	setsockopt(mClient, IPPROTO_TCP, TCP_NODELAY, (const char *)&iFlag, sizeof(iFlag));
	setsockopt(mClient, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));

	Message *m = &mMessage;
	if( connect(mClient, (struct sockaddr *)&mAddr, sizeof(mAddr)) != -1 ) {
//		LogManager::GetLogManager()->Log(LOG_STAT, "TcpTestClient::Connnect( connect ok fd : %d )", mClient);
		printf("# TcpTestClient::Connnect( connect ok fd : %d ) \n", mClient);
		char sendBuffer[2048] = {'\0'};
		memset(sendBuffer, '\0', 2048);
		snprintf(sendBuffer, 2048 - 1, "POST %s HTTP/1.0\r\nContent-Length: %d\r\nConection: %s\r\n\r\n%s",
									"/",
									(int)strlen("hello world"),
									"Keep-alive",
									"client"
									);
		send(mClient, sendBuffer, strlen(sendBuffer), 0);
		printf("# TcpTestClient::Connnect( send ok fd : %d ) \n", mClient);

		while( 1 ) {
			/* recv from call server */
			int ret = recv(mClient, m->buffer, 2048 - 1, 0);
			if( ret > 0 ) {
//				LogManager::GetLogManager()->Log(LOG_STAT, "TcpTestClient::Connnect( recv message ok )");
				printf("# TcpTestClient::Connnect( recv message ok ) \n");
				m->fd = mClient;
				m->len = ret;
				*(m->buffer + m->len) = '\0';
				if( mpTcpTestClientObserver != NULL ) {
					mpTcpTestClientObserver->OnRecvTcpTestClientMessage(this, m);
				}
			} else {
				printf("# TcpTestClient::Connnect( recv fail disconnnect ret : %d ) \n", ret);
//				LogManager::GetLogManager()->Log(LOG_STAT, "TcpTestClient::Connnect( recv fail disconnnect ret : %d )", ret);
				break;
			}
		}
	} else {
//		LogManager::GetLogManager()->Log(LOG_STAT, "TcpTestClient::Connnect( connnect fail )");
		printf("# TcpTestClient::Connnect( connnect fail )");
	}

	close(mClient);
	mIsRunning = false;
	printf("# TcpTestClient::Connnect( exit ) \n");
}
