/*
 * MatchServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "MatchServer.h"

#include <sys/syscall.h>

#define PORT 9876

/* state thread */
class StateRunnable : public KRunnable {
public:
	StateRunnable(MatchServer *container) {
		mContainer = container;
	}
	virtual ~StateRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		int iCount = 0;
		while( mContainer->IsRunning() ) {
			if ( iCount < 30 ) {
				iCount++;
			} else {
				iCount = 0;
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, GetIdleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetIdleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, GetHandleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetHandleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, GetHandleSendMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetHandleSendMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, GetWatcherList() : %d )",
						(int)syscall(SYS_gettid),
						(WatcherList*) mContainer->GetTcpServer()->GetWatcherList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
										"MatchServer::StateRunnable( tid : %d, LogManager::GetIdleMessageList() : %d )",
										(int)syscall(SYS_gettid),
										(WatcherList*) LogManager::GetLogManager()->GetIdleMessageList()->Size()
										);
			}
			sleep(1);
		}
	}
private:
	MatchServer *mContainer;
};

MatchServer::MatchServer() {
	// TODO Auto-generated constructor stub
	mpStateRunnable = new StateRunnable(this);

}

MatchServer::~MatchServer() {
	// TODO Auto-generated destructor stub
	if( mpStateRunnable ) {
		delete mpStateRunnable;
	}
}

void MatchServer::Run(int iMaxClient, int iMaxMemoryCopy, int iMaxHandleThread) {
	miMaxClient = iMaxClient;
	miMaxMemoryCopy = iMaxMemoryCopy;

	/* db manager */
	mDBManager.Init(iMaxMemoryCopy, true);

	/* request manager */
	mRequestManager.Init(&mDBManager);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( RequestManager Init ok )");

	/* match server */
	mClientTcpServer.SetTcpServerObserver(this);
	mClientTcpServer.Start(miMaxClient, PORT, iMaxHandleThread);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( TcpServer Start ok, port : %d )", PORT);

	mIsRunning = true;

	mpStateThread = new KThread(mpStateRunnable);
	if( mpStateThread->start() != -1 ) {
		printf("# Create state thread ok \n");
	}

	/* call server */
	while( true ) {
		/* do nothing here */
		sleep(3);
	}
}

bool MatchServer::IsRunning() {
	return mIsRunning;
}

TcpServer* MatchServer::GetTcpServer() {
	return &mClientTcpServer;
}

/**
 * New request
 */
bool MatchServer::OnAccept(TcpServer *ts, Message *m) {
	return true;
}

void MatchServer::OnRecvMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : %d, "
			"start "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);
	Message *sm = ts->GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		int ret = mRequestManager.HandleRecvMessage(m, sm);
		if( 0 != ret ) {
			// Process finish, send respond
			sm->fd = m->fd;
			sm->wr = m->wr;
			ts->SendMessage(sm);
		}
	} else {
		LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnRecvMessage( "
				"tid : %d, "
				"No idle message can be use "
				")",
				(int)syscall(SYS_gettid)
				);
	}
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : %d, "
			"end "
			")",
			(int)syscall(SYS_gettid),
			m->fd);
}

void MatchServer::OnSendMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnSendMessage( tid : %d, m->fd : %d, start )", (int)syscall(SYS_gettid), m->fd);
	// 发送成功，断开连接
	ts->Disconnect(m->fd, m->wr, m->ww);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnSendMessage( tid : %d, m->fd : %d, end )", (int)syscall(SYS_gettid), m->fd);
}

/**
 * OnDisconnect
 */
void MatchServer::OnDisconnect(TcpServer *ts, int fd) {
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnDisconnect( tid: %d, fd : %d )", (int)syscall(SYS_gettid), fd);
}

unsigned int MatchServer::GetTickCount() {
	timeval tNow;
	gettimeofday(&tNow, NULL);
	if (tNow.tv_usec != 0) {
		return (tNow.tv_sec * 1000 + (unsigned int)(tNow.tv_usec / 1000));
	} else{
		return (tNow.tv_sec * 1000);
	}
}
