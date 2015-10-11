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
			if ( iCount < 10 ) {
				iCount++;
			} else {
				iCount = 0;
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetIdleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetIdleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetHandleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetHandleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetSendImmediatelyMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetSendImmediatelyMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetWatcherList() : %d )",
						(int)syscall(SYS_gettid),
						(WatcherList*) mContainer->GetTcpServer()->GetWatcherList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, LogManager::GetIdleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(WatcherList*) LogManager::GetLogManager()->GetIdleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( "
						"tid : %d, "
						"GetTotal() : %u, "
						"GetHit() : %u, "
						"GetIgn() : %u "
						")",
						(int)syscall(SYS_gettid),
						mContainer->GetTotal(),
						mContainer->GetHit(),
						mContainer->GetIgn()
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
	mTotal = 0;
	mIgn = 0;
	mHit = 0;

	/* db manager */
	mDBManager.Init(iMaxMemoryCopy, false);
	mDBManager.InitSyncDataBase(4, "192.168.169.128", 3306, "qpidnetwork", "root", "123456");

	/* request manager */
	mRequestManager.Init(&mDBManager);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( RequestManager Init ok )");

	/* match server */
	mClientTcpServer.SetTcpServerObserver(this);
	mClientTcpServer.Start(miMaxClient, PORT, iMaxHandleThread);
	/**
	 * 预估相应时间,内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpServer.SetHandleSize(iMaxMemoryCopy * 5 * 10);
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
			"m->fd : [%d], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);
	Message *sm = ts->GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		sm->fd = m->fd;
		sm->wr = m->wr;
		mTotal++;
		int ret = mRequestManager.HandleRecvMessage(m, sm);
		if( 0 != ret ) {
			// Process finish, send respond
//			ts->SendMessage(sm);
			ts->SendMessageByQueue(sm);
			if( ret == -1 ) {
				mIgn++;
			} else {
				mHit++;
			}
		}
	} else {
		LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnRecvMessage( "
				"tid : %d, "
				"m->fd : [%d], "
				"No idle message can be use "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
		// 断开连接
		ts->Disconnect(m->fd);
	}
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"end "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);
}

void MatchServer::OnSendMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnSendMessage( tid : %d, m->fd : [%d], start )", (int)syscall(SYS_gettid), m->fd);
	// 发送成功，断开连接
	ts->Disconnect(m->fd);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnSendMessage( tid : %d, m->fd : [%d], end )", (int)syscall(SYS_gettid), m->fd);
}

void MatchServer::OnTimeoutMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnTimeoutMessage( tid : %d, m->fd : [%d], start )", (int)syscall(SYS_gettid), m->fd);
	Message *sm = ts->GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		sm->fd = m->fd;
		sm->wr = m->wr;
		mTotal++;
		int ret = mRequestManager.HandleTimeoutMessage(m, sm);
		// Process finish, send respond
		ts->SendMessageByQueue(sm);
		mIgn++;

	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"MatchServer::OnTimeoutMessage( "
				"tid : %d, "
				"m->fd : [%d], "
				"No idle message can be use "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
		// 断开连接
		ts->Disconnect(m->fd);
	}
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnTimeoutMessage( tid : %d, m->fd : [%d], end )", (int)syscall(SYS_gettid), m->fd);
}

/**
 * OnDisconnect
 */
void MatchServer::OnDisconnect(TcpServer *ts, int fd) {
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::OnDisconnect( tid: %d, fd : [%d] )", (int)syscall(SYS_gettid), fd);
}

unsigned int MatchServer::GetTotal() {
	return mTotal;
}

unsigned int MatchServer::GetHit() {
	return mHit;
}

unsigned int MatchServer::GetIgn() {
	return mIgn;
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
