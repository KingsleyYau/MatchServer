/*
 * TcpServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "TcpServer.h"

void accept_callback(struct ev_loop *loop, ev_io *w, int revents);
void recv_callback(struct ev_loop *loop, ev_io *w, int revents);
void send_callback(struct ev_loop *loop, ev_io *w, int revents);
void async_send_callback(struct ev_loop *loop, ev_async *w, int revents);

void accept_callback(struct ev_loop *loop, ev_io *w, int revents) {
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
			"tid : %d, "
			"fd : [%d], "
			"revents : [0x%x], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			w->fd,
			revents
			);

	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);

	int client;
	struct sockaddr_in addr;
	socklen_t iAddrLen = sizeof(struct sockaddr);
	while ( (client = accept(w->fd, (struct sockaddr *)&addr, &iAddrLen)) < 0 ) {
		if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
					"tid : %d, "
					"errno == EAGAIN ||errno == EWOULDBLOCK "
					")",
					(int)syscall(SYS_gettid)
					);
			continue;
		} else {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
					"tid : %d, "
					"accept error "
					")",
					(int)syscall(SYS_gettid)
					);
			break;
		}
	}

	int iFlag = 1;
	setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &iFlag, sizeof(iFlag));
	// closesocket（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket
	setsockopt(client, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));
	// 如果在发送数据的时，希望不经历由系统缓冲区到socket缓冲区的拷贝而影响
	int nZero = 0;
	setsockopt(client, SOL_SOCKET, SO_SNDBUF, &nZero, sizeof(nZero));

	Message *m = pTcpServer->GetIdleMessageList()->PopFront();
	if( m == NULL ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
				"tid : %d, "
				"m == NULL "
				")",
				(int)syscall(SYS_gettid)
				);
		pTcpServer->OnDisconnect(client, NULL);
		return;
	}

	m->Reset();
	m->fd = client;

	/* Add Client */
	if( pTcpServer->OnAccept(m) ) {
		WatcherList *watcherList = pTcpServer->GetWatcherList();
		if( !watcherList->Empty() ) {
			/* watch recv event for client */
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
					"tid : %d, "
					"accept client fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					client
					);

//			ev_io *watcher = watcherList->front();
			ev_io *watcher = watcherList->PopFront();
			ev_io_init(watcher, recv_callback, client, EV_READ);

			pTcpServer->LockWatcherList();
			ev_io_start(loop, watcher);
			pTcpServer->UnLockWatcherList();

			pTcpServer->GetIdleMessageList()->PushBack(m);
		} else {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
					"tid : %d, "
					"no more watcher "
					")",
					(int)syscall(SYS_gettid)
					);
			pTcpServer->OnDisconnect(client, m);
		}
	} else {
		pTcpServer->OnDisconnect(client, m);
	}

	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::accept_callback( "
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

void recv_callback(struct ev_loop *loop, ev_io *w, int revents) {
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
			"tid : %d, "
			"fd : [%d], "
			"revents : [0x%x], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			w->fd,
			revents
			);

	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);

	/* something can be recv here */
	int fd = w->fd;

	unsigned long start = pTcpServer->GetTickCount();
	do {
		Message *m = pTcpServer->GetIdleMessageList()->PopFront();
		if( m == NULL ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
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
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
					"tid : %d, "
					"fd : [%d], "
					"(revents & EV_ERROR) "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
			pTcpServer->OnDisconnect(fd, m);
			return;
		}

		int ret = recv(fd, buffer, MAXLEN - 1, 0);
		if( ret > 0 ) {
			*(buffer + ret) = '\0';
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
					"tid : %d, "
					"fd : [%d], "
					"message( ret : %d ) : \n%s\n"
					")",
					(int)syscall(SYS_gettid),
					fd,
					ret,
					buffer
					);

			/* push this message into handle queue */
			m->len = ret;
			m->type = 0;
			m->starttime = pTcpServer->GetTickCount();
			pTcpServer->GetHandleMessageList()->PushBack(m);

			if( ret != MAXLEN - 1 ) {
				break;
			}
		} else if( ret == 0 ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
					"tid : %d, "
					"fd : [%d], "
					"normal closed "
					")",
					(int)syscall(SYS_gettid),
					fd);
			pTcpServer->OnDisconnect(fd, m);
			break;
		} else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
						"tid : %d, "
						"errno == EAGAIN || errno == EWOULDBLOCK continue "
						")",
						(int)syscall(SYS_gettid)
						);
				pTcpServer->GetIdleMessageList()->PushBack(m);
				break;
			} else {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
						"tid : %d, "
						"fd : [%d], "
						"error closed "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
				pTcpServer->OnDisconnect(fd, m);
				break;
			}
		}
	} while( true );
	pTcpServer->AddRecvTime(pTcpServer->GetTickCount() - start);

	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::recv_callback( "
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

void send_callback(struct ev_loop *loop, ev_io *w, int revents) {
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
			"tid : %d, "
			"fd : [%d], "
			"revents : [0x%x], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			w->fd,
			revents
			);

	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);

	/* something can be send here */
	int fd = w->fd;

//	Message *m = pTcpServer->GetSendMessageList()->PopFront();
	Message *m = pTcpServer->GetSendMessageList()[fd].PopFront();
	if( m == NULL ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
				"tid : %d, "
				"fd : [%d], "
				"m == NULL, "
				"exit "
				")",
				(int)syscall(SYS_gettid),
				fd
				);

		/* send finish */
		WatcherList *watcherList = pTcpServer->GetWatcherList();

		pTcpServer->LockWatcherList();
		ev_io_stop(loop, w);
		pTcpServer->UnLockWatcherList();

		watcherList->PushBack(w);

		return;
	}

	m->ww = w;
	char *buffer = m->buffer;
	int len = m->len;
	int index = 0;

	if( revents & EV_ERROR ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
				"tid : %d, "
				"fd : [%d], "
				"(revents & EV_ERROR) "
				")",
				(int)syscall(SYS_gettid),
				fd
				);
		pTcpServer->OnDisconnect(fd, m);
		return;
	}

	unsigned long start = pTcpServer->GetTickCount();
	do {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
				"tid : %d, "
				"fd : [%d], "
				"message : \n%s\n "
				")",
				(int)syscall(SYS_gettid),
				fd,
				buffer + index
				);
		int ret = send(fd, buffer + index, len - index, 0);
		if( ret > 0 ) {
			index += ret;
			if( index == len ) {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
						"tid : %d, "
						"fd : [%d], "
						"send finish "
						")",
						(int)syscall(SYS_gettid),
						fd
						);

				/* send finish */
				WatcherList *watcherList = pTcpServer->GetWatcherList();

				pTcpServer->LockWatcherList();
				ev_io_stop(loop, w);
				pTcpServer->UnLockWatcherList();

				watcherList->PushBack(w);

				/* push this message into handle queue */
				m->len = 0;
				m->ww = NULL;
				pTcpServer->GetHandleMessageList()->PushBack(m);

				break;
			}
		} else if( ret == 0 ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
					"tid : %d, "
					"fd : [%d], "
					"normal closed "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
			pTcpServer->OnDisconnect(fd, m);
			break;
		} else {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
						"tid : %d, "
						"errno == EAGAIN || errno == EWOULDBLOCK continue "
						")",
						(int)syscall(SYS_gettid)
						);
				continue;
			} else {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
						"tid : %d, "
						"fd : [%d], "
						"error closed "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
				pTcpServer->OnDisconnect(fd, m);
				break;
			}
		}
	} while( true );
	pTcpServer->AddSendTime(pTcpServer->GetTickCount() - start);

//	pTcpServer->GetSendMessageList()[fd] = NULL;
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::send_callback( "
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

void async_send_callback(struct ev_loop *loop, ev_async *w, int revents) {
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
			"tid : %d, "
			"revents : [0x%x], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			revents);

	TcpServer *pTcpServer = (TcpServer *)ev_userdata(loop);

	/* watch send again */
	do {
		Message *m = pTcpServer->GetHandleSendMessageList()->PopFront();
		if( NULL == m ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
					"tid : %d, "
					"m == NULL "
					")",
					(int)syscall(SYS_gettid)
					);
			break;
		}

		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
				"tid : %d, "
				"m->fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);

		WatcherList *watcherList = pTcpServer->GetWatcherList();
		if( !watcherList->Empty() ) {
			/* watch send event for client */
//			ev_io *watcher = watcherList->front();
			ev_io *watcher = watcherList->PopFront();

//			pTcpServer->GetSendMessageList()->PushBack(m);
			pTcpServer->GetSendMessageList()[m->fd].PushBack(m);

			ev_io_init(watcher, send_callback, m->fd, EV_WRITE);

			pTcpServer->LockWatcherList();
			ev_io_start(loop, watcher);
			pTcpServer->UnLockWatcherList();

		} else {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
					"tid : %d, "
					"no more watcher "
					")",
					(int)syscall(SYS_gettid),
					revents);
		}

	} while( true );

	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::async_send_callback( "
				"tid : %d, "
				"revents : [0x%x], "
				"exit "
				")",
				(int)syscall(SYS_gettid),
				revents);
}

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

		ev_async_init(mContainer->GetAsyncSendWatcher(), async_send_callback);
		ev_async_start(mContainer->GetEvLoop(), mContainer->GetAsyncSendWatcher());

		ev_set_userdata(mContainer->GetEvLoop(), mContainer);

		ev_run(mContainer->GetEvLoop(), 0);
	}
private:
	TcpServer *mContainer;
};

/* handle thread */
class HandleRunnable : public KRunnable {
public:
	HandleRunnable(TcpServer *container) {
		mContainer = container;
	}
	virtual ~HandleRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		int count = 100;
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
					"start "
					")",
					(int)syscall(SYS_gettid),
					m->fd,
					m->len
					);

			/* handle data here */
			int len = m->len;

			if( len > 0 ) {
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
};
TcpServer::TcpServer() {
	// TODO Auto-generated constructor stub
	mIsRunning = false;
	mpTcpServerObserver = NULL;

//	mCurrentConnection = 0;
	mTotalHandleRecvTime = 0;
	mTotalHandleSendTime = 0;
	mTotalRecvTime = 0;
	mTotalSendTime = 0;

	mpHandleRunnable = new HandleRunnable(this);
	mpMainRunnable = new MainRunnable(this);
	mpSendRunnable = new SendRunnable(this);

}

TcpServer::~TcpServer() {
	// TODO Auto-generated destructor stub
	Stop();

	mpTcpServerObserver = NULL;

	if( mpMainRunnable != NULL ) {
		delete mpMainRunnable;
	}

	if( mpHandleRunnable != NULL ) {
		delete mpHandleRunnable;
	}

	if( mpSendRunnable != NULL ) {
		delete mpSendRunnable;
	}

	if( mpDisconnecting != NULL ) {
		delete mpDisconnecting;
	}

}

bool TcpServer::Start(int maxConnection, int port, int maxThreadHandle) {
//	mCurrentConnection = 0;
	mTotalHandleRecvTime = 0;
	mTotalHandleSendTime = 0;
	mTotalRecvTime = 0;
	mTotalSendTime = 0;
	miMaxThreadHandle = maxThreadHandle;
	miMaxConnection = maxConnection;

	LogManager::GetLogManager()->Log(LOG_WARNING, "TcpServer::Start( "
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
		printf("# Create socket error \n");
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
		printf("# Bind socket error \n");
		return false;
	}

	if ( listen(mServer, 1024) == -1 ) {
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "TcpServer::Start( Listen socket error )");
		printf("# Listen socket error \n");
		return false;
	}

	/* create watchers */
	for(int i = 0 ; i < 3 * maxConnection; i++) {
		ev_io *w = (ev_io *)malloc(sizeof(ev_io));
		if( w != NULL ) {
			mWatcherList.PushBack(w);
		}
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create watchers ok, mWatcherList : %d )", mWatcherList.Size());
	printf("# Create watchers ok \n");

	for(int i = 0; i < maxConnection; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create idle buffers ok )");
	printf("# Create idle buffers ok \n");

	/* init send message list */
	mpSendMessageList = new MessageList[2 * maxConnection];
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Init send message list ok )");
	printf("# Init send message list ok \n");

	mpDisconnecting = new bool[2 * maxConnection];
	for(int i = 0; i < 2 * maxConnection; i++) {
		/* init disconnecting flag */
		mpDisconnecting[i] = false;
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Init disconnecting flag ok )");
	printf("# Init disconnecting flag ok \n");

	mIsRunning = true;

	mLoop = EV_DEFAULT;

	/* start main thread */
	mpMainThread = new KThread(mpMainRunnable);
	if( mpMainThread->start() != -1 ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create main thread ok )");
		printf("# Create main thread ok \n");
	}

	/* start handle thread */
	mpHandleThread = new KThread*[miMaxThreadHandle];
	for( int i = 0; i < miMaxThreadHandle; i++ ) {
		mpHandleThread[i] = new KThread(mpHandleRunnable);
		if( mpHandleThread[i]->start() != -1 ) {
			LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create handle thread[%d] ok )", i);
			printf("# Create handle thread[%d] ok \n", i);
		}
	}

	/* start send thread */
	mpSendThread = new KThread(mpSendRunnable);
	if( mpSendThread->start() != -1 ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Start( Create send thread ok )");
		printf("# Create send thread ok \n");
	}

	return true;
}

bool TcpServer::Stop() {
	/* stop log thread */
	mIsRunning = false;

	/* release main thread */
	if( mpMainThread ) {
		mpMainThread->stop();
		delete mpMainThread;
	}

	/* release handle thread */
	for( int i = 0; i < miMaxThreadHandle; i++ ) {
		mpHandleThread[i] = new KThread(mpHandleRunnable);
		mpHandleThread[i]->stop();
		delete mpHandleThread[i];
	}
	delete mpHandleThread;

	/* release disconnecting flag */
	if( mpDisconnecting ) {
		delete mpDisconnecting;
	}

	Message *m;

	/* release send message */
	if( mpSendMessageList ) {
		for(int i = 0; i < 2 * miMaxConnection; i++) {
			while( NULL != ( m = mpSendMessageList[i].PopFront() ) ) {
				delete m;
			}
		}
		delete mpSendMessageList;
	}

	close(mServer);

	/* release log buffers */
	while( NULL != ( m = mIdleMessageList.PopFront() ) ) {
		delete m;
	}

	return true;
}

bool TcpServer::IsRunning() {
	return mIsRunning;
}

void TcpServer::SetTcpServerObserver(TcpServerObserver *observer) {
	mpTcpServerObserver = observer;
}

void TcpServer::SendMessage(Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessage( "
				"tid : %d, "
				"m->fd : [%d] "
				"ok"
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);

	/* push socket into send queue */
	mHandleSendMessageList.PushBack(m);

	LockWatcherList();
	/* signal main thread to send resopne */
	ev_async_send(mLoop, &mAsync_send_watcher);
	UnLockWatcherList();

	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessage( "
				"tid : %d, "
				"m->fd : [%d] "
				"end "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
}

void TcpServer::SendMessageByQueue(Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessageByQueue( "
				"tid : %d, "
				"m->fd : [%d] "
				"start"
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);

	GetSendImmediatelyMessageList()->PushBack(m);

	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessageByQueue( "
				"tid : %d, "
				"m->fd : [%d] "
				"end"
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
}

void TcpServer::SendMessageImmediately(Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessageImmediately( "
				"tid : %d, "
				"m->fd : [%d] "
				"start "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);

	char *buffer = m->buffer;
	int len = m->len;
	int index = 0;
	int fd = m->fd;

	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::SendMessageImmediately( "
			"tid : %d, "
			"fd : [%d], "
			"message : \n%s\n "
			")",
			(int)syscall(SYS_gettid),
			fd,
			buffer + index
			);

	do {
		int ret = send(fd, buffer + index, len - index, 0);
		if( ret > 0 ) {
			index += ret;
			if( index == len ) {
				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::SendMessageImmediately( "
						"tid : %d, "
						"fd : [%d], "
						"send finish "
						")",
						(int)syscall(SYS_gettid),
						fd
						);

				/* send finish */

				/* push this message into handle queue */
				m->len = 0;
				m->ww = NULL;
				GetHandleMessageList()->PushBack(m);

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
	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::SendMessageImmediately( "
				"tid : %d, "
				"m->fd : [%d] "
				"end "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
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
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Disconnect( "
							"tid : %d, "
							"fd : [%d], "
							"start "
							")",
							(int)syscall(SYS_gettid),
							fd
							);

	shutdown(fd, SHUT_RDWR);

	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::Disconnect( "
					"tid : %d, "
					"fd : [%d], "
					"exit "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
}

void TcpServer::StopEvio(ev_io *w) {
	if( w != NULL ) {
		LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::StopEvio( "
						"tid : %d, "
						"w != NULL "
						")",
						(int)syscall(SYS_gettid)
						);

		LockWatcherList();
		ev_io_stop(mLoop, w);
		UnLockWatcherList();

		mWatcherList.PushBack(w);
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

MessageList *TcpServer::GetHandleSendMessageList() {
	return &mHandleSendMessageList;
}

MessageList* TcpServer::GetSendMessageList() {
	return mpSendMessageList;
}

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
	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::OnDisconnect( "
							"tid : %d, "
							"fd : [%d], "
							"start "
							")",
							(int)syscall(SYS_gettid),
							fd
							);

	StopEvio(m->wr);
	StopEvio(m->ww);

	Disconnect(fd);
	close(fd);

	if( m != NULL ) {
		mIdleMessageList.PushBack(m);
	}

	LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::OnDisconnect( "
							"tid : %d, "
							"fd : [%d], "
							"end "
							")",
							(int)syscall(SYS_gettid),
							fd
							);
}

bool TcpServer::OnAccept(Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG, "TcpServer::OnAccept( "
				"tid : %d, "
				"m->fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);

	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnAccept(this, m);
	}

	return true;
}

void TcpServer::OnRecvMessage(Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG,
				"TcpServer::OnRecvMessage( "
				"tid : %d, "
				"m->fd : [%d], "
				"mTotalHandleRecvTime : %lu ms, "
				"mTotalRecvTime : %lu ms, "
				"mTotalTime : %lu ms, "
				"start "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				mTotalHandleRecvTime,
				mTotalRecvTime,
				mTotalTime
				);

	unsigned long start = GetTickCount();
	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnRecvMessage(this, m);
	}

	mTotalHandleRecvTime += GetTickCount() - start;
	mTotalTime = mTotalHandleRecvTime + mTotalRecvTime + mTotalHandleSendTime + mTotalSendTime;

	LogManager::GetLogManager()->Log(LOG_MSG,
				"TcpServer::OnRecvMessage( "
				"tid : %d, "
				"m->fd : [%d], "
				"mTotalHandleRecvTime : %lu ms, "
				"mTotalRecvTime : %lu ms, "
				"mTotalTime : %lu ms, "
				"end "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				mTotalHandleRecvTime,
				mTotalRecvTime,
				mTotalTime
				);
}

void TcpServer::OnSendMessage(Message *m) {
	unsigned long start = GetTickCount();

	mTotalHandleSendTime += GetTickCount() - start;
	mTotalTime = mTotalHandleRecvTime + mTotalRecvTime + mTotalHandleSendTime + mTotalSendTime;

	LogManager::GetLogManager()->Log(LOG_MSG,
			"TcpServer::OnSendMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"mTotalHandleSendTime : %lu ms, "
			"mTotalSendTime : %lu ms, "
			"mTotalTime : %lu ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			mTotalHandleSendTime,
			mTotalSendTime,
			mTotalTime
			);

	if( mpTcpServerObserver != NULL ) {
		mpTcpServerObserver->OnSendMessage(this, m);
	}
}

unsigned int TcpServer::GetTickCount() {
	timeval tNow;
	gettimeofday(&tNow, NULL);
	if (tNow.tv_usec != 0) {
		return (tNow.tv_sec * 1000 + (unsigned int)(tNow.tv_usec / 1000));
	} else{
		return (tNow.tv_sec * 1000);
	}
}

void TcpServer::AddRecvTime(unsigned long time) {
	mTotalRecvTime += time;
}

void TcpServer::AddSendTime(unsigned long time) {
	mTotalSendTime += time;
}

void TcpServer::LockWatcherList() {
	mWatcherListMutex.lock();
}

void TcpServer::UnLockWatcherList() {
	mWatcherListMutex.unlock();
}

void TcpServer::HandleCloseQueue() {
	TCloseSocketItem item;

	while ( true ) {
		mCloseSocketQueueMutex.lock();
		if( !mCloseSocketQueue.empty() ) {
			item = mCloseSocketQueue.front();
			mCloseSocketQueue.pop_front();
			mCloseSocketQueueMutex.unlock();

			if ( DiffGetTickCount(item.tApplyCloseTime, GetTickCount()) >= CLOSESOCKET_TIME ) {

				LogManager::GetLogManager()->Log(LOG_STAT, "TcpServer::HandleCloseQueue( tid : %d, fd : [%d], timeout )",
														(int)syscall(SYS_gettid),
														item.iFd
														);

				if( mpTcpServerObserver != NULL ) {
					mpTcpServerObserver->OnDisconnect(this, item.iFd);
				}

				mDisconnectingMutex.lock();
				if( mpDisconnecting[item.iFd] ) {
					mpDisconnecting[item.iFd] = false;
					mDisconnectingMutex.unlock();
				} else {
					mDisconnectingMutex.unlock();
				}

				close(item.iFd);
			} else {
				// 没到时间
				break;
			}
		} else {
			mCloseSocketQueueMutex.unlock();
			break;
		}
	}
}
