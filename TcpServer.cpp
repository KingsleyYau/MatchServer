/*
 * TcpServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "TcpServer.h"

#include <common/Arithmetic.hpp>
#include <common/TimeProc.hpp>

//static struct ev_loop *gLoop = NULL;

void accept_callback(struct ev_loop *loop, ev_io *w, int revents);
void recv_callback(struct ev_loop *loop, ev_io *w, int revents);
//void send_callback(struct ev_loop *loop, ev_io *w, int revents);
//void async_send_callback(struct ev_loop *loop, ev_async *w, int revents);

void accept_callback(struct ev_loop *loop, ev_io *w, int revents) {
	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);
	pTcpServer->Accept_Callback(w, revents);
}

void recv_callback(struct ev_loop *loop, ev_io *w, int revents) {
	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);
	pTcpServer->Recv_Callback(w, revents);
}

//void send_callback(struct ev_loop *loop, ev_io *w, int revents) {
//	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//			"tid : %d, "
//			"fd : [%d], "
//			"revents : [0x%x], "
//			"start "
//			")",
//			(int)syscall(SYS_gettid),
//			w->fd,
//			revents
//			);
//
//	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);
//
//	/* something can be send here */
//	int fd = w->fd;
//
////	Message *m = pTcpServer->GetSendMessageList()->PopFront();
//	Message *m = pTcpServer->GetSendMessageList()[fd].PopFront();
//	if( m == NULL ) {
//		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//				"tid : %d, "
//				"fd : [%d], "
//				"m == NULL, "
//				"exit "
//				")",
//				(int)syscall(SYS_gettid),
//				fd
//				);
//
//		/* send finish */
//		WatcherList *watcherList = pTcpServer->GetWatcherList();
//
//		pTcpServer->LockWatcherList();
//		ev_io_stop(loop, w);
//		pTcpServer->UnLockWatcherList();
//
//		watcherList->PushBack(w);
//
//		return;
//	}
//
//	m->ww = w;
//	char *buffer = m->buffer;
//	int len = m->len;
//	int index = 0;
//
//	if( revents & EV_ERROR ) {
//		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//				"tid : %d, "
//				"fd : [%d], "
//				"(revents & EV_ERROR) "
//				")",
//				(int)syscall(SYS_gettid),
//				fd
//				);
//		pTcpServer->OnDisconnect(fd, m);
//		return;
//	}
//
//	unsigned long start = GetTickCount();
//	do {
//		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//				"tid : %d, "
//				"fd : [%d], "
//				"message : \n%s\n "
//				")",
//				(int)syscall(SYS_gettid),
//				fd,
//				buffer + index
//				);
//		int ret = send(fd, buffer + index, len - index, 0);
//		if( ret > 0 ) {
//			index += ret;
//			if( index == len ) {
//				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//						"tid : %d, "
//						"fd : [%d], "
//						"send finish "
//						")",
//						(int)syscall(SYS_gettid),
//						fd
//						);
//
//				/* send finish */
//				WatcherList *watcherList = pTcpServer->GetWatcherList();
//
//				pTcpServer->LockWatcherList();
//				ev_io_stop(loop, w);
//				pTcpServer->UnLockWatcherList();
//
//				watcherList->PushBack(w);
//
//				/* push this message into handle queue */
//				m->len = 0;
//				m->ww = NULL;
//				pTcpServer->GetHandleMessageList()->PushBack(m);
//
//				break;
//			}
//		} else if( ret == 0 ) {
//			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//					"tid : %d, "
//					"fd : [%d], "
//					"normal closed "
//					")",
//					(int)syscall(SYS_gettid),
//					fd
//					);
//			pTcpServer->OnDisconnect(fd, m);
//			break;
//		} else {
//			if(errno == EAGAIN || errno == EWOULDBLOCK) {
//				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//						"tid : %d, "
//						"errno == EAGAIN || errno == EWOULDBLOCK continue "
//						")",
//						(int)syscall(SYS_gettid)
//						);
//				continue;
//			} else {
//				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//						"tid : %d, "
//						"fd : [%d], "
//						"error closed "
//						")",
//						(int)syscall(SYS_gettid),
//						fd
//						);
//				pTcpServer->OnDisconnect(fd, m);
//				break;
//			}
//		}
//	} while( true );
//	pTcpServer->AddSendTime(GetTickCount() - start);
//
////	pTcpServer->GetSendMessageList()[fd] = NULL;
//	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
//			"tid : %d, "
//			"fd : [%d], "
//			"revents : [0x%x], "
//			"exit "
//			")",
//			(int)syscall(SYS_gettid),
//			w->fd,
//			revents
//			);
//}
//
//void async_send_callback(struct ev_loop *loop, ev_async *w, int revents) {
//	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
//			"tid : %d, "
//			"revents : [0x%x], "
//			"start "
//			")",
//			(int)syscall(SYS_gettid),
//			revents);
//
//	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);
//
//	/* watch send again */
//	do {
//		Message *m = pTcpServer->GetHandleSendMessageList()->PopFront();
//		if( NULL == m ) {
//			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
//					"tid : %d, "
//					"m == NULL "
//					")",
//					(int)syscall(SYS_gettid)
//					);
//			break;
//		}
//
//		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
//				"tid : %d, "
//				"m->fd : [%d] "
//				")",
//				(int)syscall(SYS_gettid),
//				m->fd
//				);
//
//		WatcherList *watcherList = pTcpServer->GetWatcherList();
//		if( !watcherList->Empty() ) {
//			/* watch send event for client */
////			ev_io *watcher = watcherList->front();
//			ev_io *watcher = watcherList->PopFront();
//
////			pTcpServer->GetSendMessageList()->PushBack(m);
//			pTcpServer->GetSendMessageList()[m->fd].PushBack(m);
//
//			ev_io_init(watcher, send_callback, m->fd, EV_WRITE);
//
//			pTcpServer->LockWatcherList();
//			ev_io_start(loop, watcher);
//			pTcpServer->UnLockWatcherList();
//
//		} else {
//			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
//					"tid : %d, "
//					"no more watcher "
//					")",
//					(int)syscall(SYS_gettid),
//					revents);
//		}
//
//	} while( true );
//
//	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
//				"tid : %d, "
//				"revents : [0x%x], "
//				"exit "
//				")",
//				(int)syscall(SYS_gettid),
//				revents);
//}

/* send thread */
class SendRunnable : public KRunnable {
public:
	SendRunnable(TcpServer *container) {
		mContainer = container;
	}
	virtual ~SendRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		while( mContainer->IsRunning() ) {
			mContainer->SendAllMessageImmediately();
			usleep(100000);
		}
	}
private:
	TcpServer *mContainer;
};

/* accept thread */
class MainRunnable : public KRunnable {
public:
	MainRunnable(TcpServer *container) {
		mContainer = container;
	}
	virtual ~MainRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		/* run main loop */

		ev_io_init(mContainer->GetAcceptWatcher(), accept_callback, mContainer->GetSocket(), EV_READ);
		ev_io_start(mContainer->GetEvLoop(), mContainer->GetAcceptWatcher());

//		ev_async_init(mContainer->GetAsyncSendWatcher(), async_send_callback);
//		ev_async_start(mContainer->GetEvLoop(), mContainer->GetAsyncSendWatcher());

		ev_set_userdata(mContainer->GetEvLoop(), mContainer);

		ev_run(mContainer->GetEvLoop(), 0);
	}
private:
	TcpServer *mContainer;
};

/* handle thread */
class HandleRunnable : public KRunnable {
public:
	HandleRunnable(TcpServer *container, int index) {
		mContainer = container;
		mIndex = index;
	}
	virtual ~HandleRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		while( true ) {
			Message *m = mContainer->GetHandleMessageList()->PopFront();
			if( NULL == m ) {
				if( !mContainer->IsRunning() ) {
					/* all message handle break */
					break;
				}

				usleep(100000);
				continue;
			}

			LogManager::GetLogManager()->Log(LOG_STAT, "HandleRunnable::onRun( "
					"tid: %d, "
					"m->fd: [%d], "
					"m->len : %d, "
					"mIndex : %d "
					"start "
					")",
					(int)syscall(SYS_gettid),
					m->fd,
					m->len,
					mIndex
					);

			/* handle data here */
			m->index = mIndex;

			if( m->type == MessageTypeRecv ) {
				/* recv message ok */
				mContainer->OnRecvMessage(m);
			} else {
				/* send message ok */
				mContainer->OnSendMessage(m);
			}

			/* push into idle queue */
			LogManager::GetLogManager()->Log(LOG_STAT, "HandleRunnable::onRun( "
					"tid: %d, "
					"m->fd: [%d], "
					"m->len : %d, "
					"handle buffer finish, push into idle queue, exit "
					")",
					(int)syscall(SYS_gettid),
					m->fd,
					m->len
					);
			mContainer->GetIdleMessageList()->PushBack(m);
		}
	}

	TcpServer *mContainer;
	int mIndex;

};
TcpServer::TcpServer() {
	// TODO Auto-generated constructor stub
	mLoop = NULL;
	mIsRunning = false;
	mpTcpServerObserver = NULL;
	mServer = 0;
	miMaxConnection = 0;
	mHandleSize = 0;

	mpMainThread = NULL;

	miMaxThreadHandle = 0;
	mpHandleRunnable = NULL;
	mpHandleThread = NULL;

	mpMainRunnable = new MainRunnable(this);

	mpSendRunnable = new SendRunnable(this);
	mpSendThread = NULL;

	mpHandlingMessageCount = NULL;
	mpCloseRecv = NULL;
	mpPacketSeqRecv = NULL;
	mpPacketSeqSend = NULL;

}

TcpServer::~TcpServer() {
	// TODO Auto-generated destructor stub
	Stop();

	mpTcpServerObserver = NULL;

	if( mpMainRunnable != NULL ) {
		delete mpMainRunnable;
	}

	if( mpSendRunnable != NULL ) {
		delete mpSendRunnable;
	}
}

bool TcpServer::Start(int maxConnection, int port, int maxThreadHandle) {
	if( mIsRunning ) {
		return false;
	}

	printf("# TcpServer starting... \n");

	miMaxThreadHandle = maxThreadHandle;
	miMaxConnection = maxConnection;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::Start( "
			"maxConnection : %d, "
			"maxThreadHandle : %d "
			" )",
			maxConnection,
			maxThreadHandle
			);

	/* bind server socket */
	struct sockaddr_in ac_addr;
	if ((mServer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "TcpServer::Start( Create socket error )");
		printf("# Create socket error! \n");
		return -1;
	}

	int so_reuseaddr = 1;
	setsockopt(mServer, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));

	bzero(&ac_addr, sizeof(ac_addr));
	ac_addr.sin_family = PF_INET;
	ac_addr.sin_port = htons(port);
	ac_addr.sin_addr.s_addr = INADDR_ANY;

	if ( bind(mServer, (struct sockaddr *) &ac_addr, sizeof(struct sockaddr))== -1 ) {
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "TcpServer::Start( Bind socket error )");
		printf("# Bind socket error! \n");
		return false;
	}

	if ( listen(mServer, 1024) == -1 ) {
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "TcpServer::Start( Listen socket error )");
		printf("# Listen socket error! \n");
		return false;
	}

	mpCloseRecv = new bool[2 * maxConnection];
	mpHandlingMessageCount = new int[2 * maxConnection];
	mpPacketSeqRecv = new int[2 * maxConnection];
	mpPacketSeqSend = new int[2 * maxConnection];

	/* create watchers */
	for(int i = 0 ; i < 2 * maxConnection; i++) {
		ev_io *w = (ev_io *)malloc(sizeof(ev_io));
		if( w != NULL ) {
			mWatcherList.PushBack(w);
		}
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create watchers ok, mWatcherList : %d )", mWatcherList.Size());

	for(int i = 0; i < 10000; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create idle buffers ok )");

	mIsRunning = true;

	mLoop = ev_loop_new(EVFLAG_AUTO);//EV_DEFAULT;

	/* start main thread */
	mpMainThread = new KThread(mpMainRunnable);
	if( mpMainThread->start() != 0 ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create main thread ok )");
	}

	/* start handle thread */
	mpHandleThread = new KThread*[miMaxThreadHandle];
	mpHandleRunnable = new HandleRunnable*[miMaxThreadHandle];
	for( int i = 0; i < miMaxThreadHandle; i++ ) {
		mpHandleRunnable[i] = new HandleRunnable(this, i);
		mpHandleThread[i] = new KThread(mpHandleRunnable[i]);
		if( mpHandleThread[i]->start() != 0 ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create handle thread[%d] ok )", i);
		}
	}

	/* start send thread */
	mpSendThread = new KThread(mpSendRunnable);
	if( mpSendThread->start() != 0 ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create send thread ok )");
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( start ok )");
	printf("# TcpServer start OK. \n");
	return true;
}

bool TcpServer::Stop() {
	if( mIsRunning ) {
		mIsRunning = false;
	} else {
		return false;
	}

	printf("# TcpServer stopping... \n");

	/* release main thread */
	if( mpMainThread ) {
		mpMainThread->stop();
		delete mpMainThread;
	}

	/* release handle thread */
	if( mpHandleThread != NULL && mpHandleThread != NULL ) {
		for( int i = 0; i < miMaxThreadHandle; i++ ) {
			if( mpHandleThread[i] != NULL ) {
				mpHandleThread[i]->stop();
				delete mpHandleThread[i];
			}
			if( mpHandleRunnable[i] != NULL ) {
				delete mpHandleRunnable[i];
			}
		}
		delete mpHandleThread;
		delete mpHandleRunnable;
	}

	close(mServer);

	if( mpCloseRecv != NULL ) {
		delete mpCloseRecv;
	}

	if( mpHandlingMessageCount != NULL ) {
		delete mpHandlingMessageCount;
	}

	if( mpPacketSeqRecv != NULL ) {
		delete mpPacketSeqRecv;
	}

	if( mpPacketSeqSend != NULL ) {
		delete mpPacketSeqSend;
	}

	Message *m;

	/* release send message */
	while( NULL != ( m = mSendImmediatelyMessageList.PopFront() ) ) {
		delete m;
	}

	/* release log buffers */
	while( NULL != ( m = mIdleMessageList.PopFront() ) ) {
		delete m;
	}

	if( mLoop ) {
		ev_loop_destroy(mLoop);
	}

	printf("# TcpServer stop OK. \n");

	return true;
}

bool TcpServer::IsRunning() {
	return mIsRunning;
}

void TcpServer::SetTcpServerObserver(TcpServerObserver *observer) {
	mpTcpServerObserver = observer;
}

void TcpServer::Accept_Callback(ev_io *w, int revents) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::Accept_Callback( "
			"tid : %d, "
			"fd : [%d], "
			"revents : [0x%x], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			w->fd,
			revents
			);

	int client = 0;
	struct sockaddr_in addr;
	socklen_t iAddrLen = sizeof(struct sockaddr);
	while ( (client = accept(w->fd, (struct sockaddr *)&addr, &iAddrLen)) < 0 ) {
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::Accept_Callback( "
					"tid : %d, "
					"fd : [%d], "
					"errno == EAGAIN ||errno == EWOULDBLOCK "
					")",
					(int)syscall(SYS_gettid),
					w->fd
					);
			continue;

		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"TcpServer::Accept_Callback( "
					"tid : %d, "
					"fd : [%d], "
					"accept error "
					")",
					(int)syscall(SYS_gettid),
					w->fd
					);
			break;
		}
	}

	if( client == 0 ) {
		return;
	}

	mCloseMutex.lock();
	mpCloseRecv[client] = false;
	mpHandlingMessageCount[client] = 0;
	mCloseMutex.unlock();

	mpPacketSeqRecvMutex.lock();
	mpPacketSeqRecv[client] = 0;
	mpPacketSeqRecvMutex.unlock();

	mpPacketSeqSendMutex.lock();
	mpPacketSeqSend[client] = 0;
	mpPacketSeqSendMutex.unlock();

	int iFlag = 1;
	setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &iFlag, sizeof(iFlag));
	// CloseSocketIfNeed（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket
	setsockopt(client, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));
	// 如果在发送数据的时，希望不经历由系统缓冲区到socket缓冲区的拷贝而影响
	int nZero = 0;
	setsockopt(client, SOL_SOCKET, SO_SNDBUF, &nZero, sizeof(nZero));

    /*deal with the tcp keepalive
      iKeepAlive = 1 (check keepalive)
      iKeepIdle = 600 (active keepalive after socket has idled for 10 minutes)
      KeepInt = 60 (send keepalive every 1 minute after keepalive was actived)
      iKeepCount = 3 (send keepalive 3 times before disconnect from peer)
     */
    int iKeepAlive = 1, iKeepIdle = 60, KeepInt = 20, iKeepCount = 3;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive));
    setsockopt(client, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&iKeepIdle, sizeof(iKeepIdle));
    setsockopt(client, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&KeepInt, sizeof(KeepInt));
    setsockopt(client, IPPROTO_TCP, TCP_KEEPCNT, (void *)&iKeepCount, sizeof(iKeepCount));

	char* ip = inet_ntoa(addr.sin_addr);

//	Message *m = GetIdleMessageList()->PopFront();
//	if( m == NULL ) {
//		LogManager::GetLogManager()->Log(LOG_WARNING, "TcpServer::Accept_Callback( "
//				"tid : %d, "
//				"accept client fd : [%d], "
//				"m == NULL "
//				")",
//				(int)syscall(SYS_gettid),
//				client
//				);
//		OnDisconnect(client, NULL);
//		return;
//	}

//	m->Reset();
//	m->fd = client;

	/* Add Client */
	if( OnAccept(client, ip) ) {
		WatcherList *watcherList = GetWatcherList();
		if( !watcherList->Empty() ) {
			/* watch recv event for client */
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::Accept_Callback( "
					"tid : %d, "
					"accept client fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					client
					);

			ev_io *watcher = watcherList->PopFront();
			ev_io_init(watcher, recv_callback, client, EV_READ);

			LockWatcherList();
			ev_io_start(mLoop, watcher);
			UnLockWatcherList();

//			GetIdleMessageList()->PushBack(m);
		} else {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"TcpServer::Accept_Callback( "
					"tid : %d, "
					"no more watcher "
					")",
					(int)syscall(SYS_gettid)
					);
			Disconnect(client);
			OnDisconnect(client, NULL);

		}
	} else {
		Disconnect(client);
		OnDisconnect(client, NULL);

	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::Accept_Callback( "
				"tid : %d, "
				"fd : [%d], "
				"revents : [0x%x], "
				"exit "
				")",
				(int)syscall(SYS_gettid),
				w->fd,
				revents
				);
}

void TcpServer::Recv_Callback(ev_io *w, int revents) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::Recv_Callback( "
			"tid : %d, "
			"fd : [%d], "
			"revents : [0x%x], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			w->fd,
			revents
			);

	/* something can be recv here */
	int fd = w->fd;

	do {
		Message *m = GetIdleMessageList()->PopFront();
		if( m == NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"TcpServer::Recv_Callback( "
					"tid : %d, "
					"fd : [%d], "
					"m == NULL, "
					"continue "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
			sleep(1);
			continue;
		}

		m->Reset();
		m->fd = fd;
		m->wr = w;
		char *buffer = m->buffer;

		if( revents & EV_ERROR ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::Recv_Callback( "
					"tid : %d, "
					"fd : [%d], "
					"(revents & EV_ERROR) "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
			OnDisconnect(fd, m);
			return;
		}

		int ret = recv(fd, buffer, MAX_BUFFER_LEN - 1, 0);
		if( ret > 0 ) {
			*(buffer + ret) = '\0';
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::Recv_Callback( "
					"tid : %d, "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd,
					ret
					);

			/* push this message into handle queue */
			m->len = ret;
			m->type = MessageTypeRecv;
			m->starttime = GetTickCount();

			if ( GetHandleSize() > 0 &&
					GetHandleMessageList()->Size() > GetHandleSize() ) {
				// over handle queue max size, maybe timeout, return at once
				OnTimeoutMessage(m);
				GetIdleMessageList()->PushBack(m);
			} else {
				mCloseMutex.lock();
				mpHandlingMessageCount[fd]++;
				mCloseMutex.unlock();

				mpPacketSeqRecvMutex.lock();
				m->seq = mpPacketSeqRecv[fd]++;
				mpPacketSeqRecvMutex.unlock();

				GetHandleMessageList()->PushBack(m);
			}

			if( ret != MAX_BUFFER_LEN - 1 ) {
				break;
			}

		} else if( ret == 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::Recv_Callback( "
					"tid : %d, "
					"fd : [%d], "
					"normal closed "
					")",
					(int)syscall(SYS_gettid),
					fd);
			OnDisconnect(fd, m);
			break;

		} else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"TcpServer::Recv_Callback( "
						"tid : %d, "
						"errno == EAGAIN || errno == EWOULDBLOCK continue "
						")",
						(int)syscall(SYS_gettid)
						);
				GetIdleMessageList()->PushBack(m);
				break;

			} else {
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"TcpServer::Recv_Callback( "
						"tid : %d, "
						"fd : [%d], "
						"error closed "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
				OnDisconnect(fd, m);
				break;

			}
		}
	} while( true );

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::Recv_Callback( "
			"tid : %d, "
			"fd : [%d], "
			"revents : [0x%x], "
			"exit "
			")",
			(int)syscall(SYS_gettid),
			w->fd,
			revents
			);
}


//void TcpServer::SendMessage(Message *m) {
////	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessage( "
////				"tid : %d, "
////				"m->fd : [%d] "
////				"ok"
////				")",
////				(int)syscall(SYS_gettid),
////				m->fd
////				);
////
////	/* push socket into send queue */
////	mHandleSendMessageList.PushBack(m);
////
////	LockWatcherList();
////	/* signal main thread to send resopne */
////	ev_async_send(GetEvLoop(), &mAsync_send_watcher);
////	UnLockWatcherList();
////
////	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessage( "
////				"tid : %d, "
////				"m->fd : [%d] "
////				"end "
////				")",
////				(int)syscall(SYS_gettid),
////				m->fd
////				);
//}

void TcpServer::SendMessageByQueue(Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::SendMessageByQueue( "
			"tid : %d, "
			"m->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	mCloseMutex.lock();
	mpHandlingMessageCount[m->fd]++;
	mCloseMutex.unlock();

	mpPacketSeqSendMutex.lock();
	m->seq = mpPacketSeqSend[m->fd]++;
	mpPacketSeqSendMutex.unlock();

	GetSendImmediatelyMessageList()->PushBack(m);

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::SendMessageByQueue( "
			"tid : %d, "
			"m->fd : [%d] "
			"end "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

}

void TcpServer::SendMessageImmediately(Message *m) {
	char *buffer = m->buffer;
	int len = m->len;
	int index = 0;
	int fd = m->fd;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::SendMessageImmediately( "
			"tid : %d, "
			"fd : [%d], "
			"buffer( len : %d ) "
			")",
			(int)syscall(SYS_gettid),
			fd,
			len
			);

	Arithmetic ari;
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::SendMessageImmediately( "
			"tid : %d, "
			"fd : [%d], "
			"buffer( len : %d ) : [\n%s\n]"
			")",
			(int)syscall(SYS_gettid),
			fd,
			len,
			ari.AsciiToHexWithSep(buffer, len).c_str()
			);
	do {
		int ret = send(fd, buffer + index, len - index, 0);
		if( ret > 0 ) {
			index += ret;
			if( index == len ) {
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"TcpServer::SendMessageImmediately( "
						"tid : %d, "
						"fd : [%d], "
						"send finish "
						")",
						(int)syscall(SYS_gettid),
						fd
						);

				/* send finish */
				m->len = 0;
				m->ww = NULL;

				/* push this message into handle queue */
//				GetHandleMessageList()->PushBack(m);
				/* push this message into idle queue */
				OnSendMessage(m);
				GetIdleMessageList()->PushBack(m);

				break;
			}
		} else if( ret == 0 ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::SendMessageImmediately( "
					"tid : %d, "
					"fd : [%d], "
					"normal closed "
					")",
					(int)syscall(SYS_gettid),
					m->fd
					);
			Disconnect(fd);
			GetIdleMessageList()->PushBack(m);
			break;
		} else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::SendMessageImmediately( "
						"tid : %d, "
						"errno == EAGAIN || errno == EWOULDBLOCK continue "
						")",
						(int)syscall(SYS_gettid)
						);
				continue;
			} else {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::SendMessageImmediately( "
						"tid : %d, "
						"fd : [%d], "
						"error closed "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
				Disconnect(fd);
				GetIdleMessageList()->PushBack(m);
				break;
			}
		}
	} while(true);

	CloseSocketIfNeedByHandleThread(m->fd);

}

void TcpServer::SendAllMessageImmediately() {
	do {
		Message *m = GetSendImmediatelyMessageList()->PopFront();
		if( m == NULL ) {
			break;
		}

		SendMessageImmediately(m);
	}while(true);
}

void TcpServer::Disconnect(int fd) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::Disconnect( "
			"tid : %d, "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			fd
			);

	shutdown(fd, SHUT_RD);

}

void TcpServer::StopEvio(ev_io *w) {
	if( w != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"TcpServer::StopEvio( "
				"tid : %d, "
				"w != NULL "
				")",
				(int)syscall(SYS_gettid)
				);

		LockWatcherList();
		ev_io_stop(GetEvLoop(), w);
		UnLockWatcherList();

		mWatcherList.PushBack(w);
	}
}

void TcpServer::CloseSocketIfNeedByHandleThread(int fd) {
	bool bClose = false;
	mCloseMutex.lock();
	--mpHandlingMessageCount[fd];

	LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::CloseSocketIfNeedByHandleThread( "
					"tid : %d, "
					"fd : [%d], "
					"mpHandlingMessageCount[%d] : %d "
					")",
					(int)syscall(SYS_gettid),
					fd,
					fd,
					mpHandlingMessageCount[fd]
					);

	if( mpHandlingMessageCount[fd] == 0 ) {
		if( mpCloseRecv[fd] ) {
			LogManager::GetLogManager()->Log(
							LOG_MSG,
							"TcpServer::CloseSocketIfNeedByHandleThread( "
							"tid : %d, "
							"fd : [%d], "
							"close ok "
							")",
							(int)syscall(SYS_gettid),
							fd
							);

			bClose = true;

		}
	}
	mCloseMutex.unlock();

	if( bClose ) {
		if( mpTcpServerObserver != NULL ) {
			mpTcpServerObserver->OnClose(this, fd);
		}

		close(fd);
	}
}

MessageList *TcpServer::GetIdleMessageList() {
	return &mIdleMessageList;
}

MessageList *TcpServer::GetHandleMessageList() {
	return &mHandleMessageList;
}

MessageList *TcpServer::GetSendImmediatelyMessageList() {
	return &mSendImmediatelyMessageList;
}

//MessageList *TcpServer::GetHandleSendMessageList() {
//	return &mHandleSendMessageList;
//}
//
//MessageList* TcpServer::GetSendMessageList() {
//	return mpSendMessageList;
//}

WatcherList *TcpServer::GetWatcherList() {
	return &mWatcherList;
}

int TcpServer::GetSocket() {
	return mServer;
}

ev_io *TcpServer::GetAcceptWatcher() {
	return &mAccept_watcher;
}

ev_async *TcpServer::GetAsyncSendWatcher() {
	return &mAsync_send_watcher;
}

struct ev_loop *TcpServer::GetEvLoop() {
	return mLoop;
}

void TcpServer::OnDisconnect(int fd, Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::OnDisconnect( "
			"tid : %d, "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			fd
			);

	if( m != NULL ) {
		StopEvio(m->wr);
		StopEvio(m->ww);
	}

	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnDisconnect(this, fd);
	}

	bool bClose = false;
	mCloseMutex.lock();
	LogManager::GetLogManager()->Log(
					LOG_STAT,
					"TcpServer::OnDisconnect( "
					"tid : %d, "
					"fd : [%d], "
					"mpHandlingMessageCount[%d] : %d "
					")",
					(int)syscall(SYS_gettid),
					fd,
					fd,
					mpHandlingMessageCount[fd]
					);

	if( mpHandlingMessageCount[fd] == 0 ) {
		LogManager::GetLogManager()->Log(
						LOG_MSG,
						"TcpServer::OnDisconnect( "
						"tid : %d, "
						"fd : [%d], "
						"close ok "
						")",
						(int)syscall(SYS_gettid),
						fd,
						fd,
						mpHandlingMessageCount[fd]
						);

		bClose = true;

	} else {
		mpCloseRecv[fd] = true;

	}
	mCloseMutex.unlock();

	if( bClose ) {
		if( mpTcpServerObserver != NULL ) {
			mpTcpServerObserver->OnClose(this, fd);
		}

		close(fd);
	}

	if( m != NULL ) {
		mIdleMessageList.PushBack(m);
	}

}

bool TcpServer::OnAccept(int fd, char* ip) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::OnAccept( "
			"tid : %d, "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			fd
			);

	bool bFlag = true;

	if( mpTcpServerObserver != NULL ) {
		bFlag = mpTcpServerObserver->OnAccept(this, fd, ip);
	}

	return bFlag;
}

void TcpServer::OnRecvMessage(Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"m->seq : %d, "
			"len : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			m->seq,
			m->len
			);

	Arithmetic ari;
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"TcpServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"m->seq : %d, "
			"buffer : [\n%s\n] "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			m->seq,
			ari.AsciiToHexWithSep(m->buffer, m->len).c_str()
			);

	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnRecvMessage(this, m);
	}

	CloseSocketIfNeedByHandleThread(m->fd);

}

void TcpServer::OnSendMessage(Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::OnSendMessage( "
			"tid : %d, "
			"m->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnSendMessage(this, m);
	}
}

void TcpServer::OnTimeoutMessage(Message *m) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"TcpServer::OnTimeoutMessage( "
			"tid : %d, "
			"m->fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);

	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnTimeoutMessage(this, m);
	}
}

//void TcpServer::AddRecvTime(unsigned long time) {
//	mTotalRecvTime += time;
//}
//
//void TcpServer::AddSendTime(unsigned long time) {
//	mTotalSendTime += time;
//}

void TcpServer::LockWatcherList() {
	mWatcherListMutex.lock();
}

void TcpServer::UnLockWatcherList() {
	mWatcherListMutex.unlock();
}

void TcpServer::SetHandleSize(unsigned int size) {
	mHandleSize = size;
}

unsigned int TcpServer::GetHandleSize() {
	return mHandleSize;
}
