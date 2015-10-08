/*
 * TcpServer.h
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include <ev.h>

#include <deque>

#include "KThread.h"
#include "MessageList.h"
#include "LogManager.h"
#include "KSafeList.h"

#define CLOSESOCKET_TIME    1000     // 1秒后关闭
#define DiffGetTickCount(start, end)    ((start) <= (end) ? (end) - (start) : ((unsigned int)(-1)) - (start) + (end))

//typedef list<ev_io *> WatcherList;
typedef KSafeList<ev_io *> WatcherList;

class CloseSocketRunnable;
class HandleRunnable;
class MainRunnable;
class TcpServer;

/// -----=== fgx 2012-12-12 ===----
/// 延迟closesocket，延长socket值重用时间
typedef struct _tagCloseSocketItem {
    int     iFd;
    time_t  tApplyCloseTime;
} TCloseSocketItem;
typedef deque<TCloseSocketItem> CloseSocketQueue;

class TcpServerObserver {
public:
	virtual bool OnAccept(TcpServer *ts, Message *m) = 0;
	virtual void OnRecvMessage(TcpServer *ts, Message *m) = 0;
	virtual void OnSendMessage(TcpServer *ts, Message *m) = 0;
	virtual void OnDisconnect(TcpServer *ts, int fd) = 0;
};
class TcpServer {
public:
	TcpServer();
	virtual ~TcpServer();

	bool Start(int maxConnection, int port, int maxThreadHandle = 1);
	bool Stop();
	bool IsRunning();
	void SetTcpServerObserver(TcpServerObserver *observer);

	void SendMessage(Message *m);

	bool Disconnect(int fd, ev_io *wr, ev_io *ww);

	int GetSocket();
	MessageList *GetIdleMessageList();
	MessageList *GetHandleMessageList();
	MessageList* GetSendMessageList();
	MessageList *GetHandleSendMessageList();
//	Message** GetSendMessageList();

	WatcherList *GetWatcherList();
	ev_async *GetAsyncSendWatcher();
	ev_io *GetAcceptWatcher();
	struct ev_loop *GetEvLoop();

	bool OnAccept(Message *m);
	void OnRecvMessage(Message *m);
	void OnSendMessage(Message *m);
	void OnDisconnect(int fd, Message *m);

	void AddRecvTime(unsigned long time);
	void AddSendTime(unsigned long time);

	unsigned int GetTickCount();

	void HandleCloseQueue();

	void LockWatcherList();
	void UnLockWatcherList();
private:
	/* Thread safe message list */
	MessageList mIdleMessageList;
	MessageList mHandleMessageList;

//	MessageList mSendMessageList;
	MessageList mHandleSendMessageList;
	MessageList* mpSendMessageList;

//	Message **mpSendMessageList;

	/* Thread safe watcher list */
	WatcherList mWatcherList;
	KMutex mWatcherListMutex;

	/**
	 * CloseSocket线程
	 */
	CloseSocketRunnable* mpCloseSocketRunnable;
	KThread* mpCloseSocketThread;

	bool* mpDisconnecting;
	KMutex mDisconnectingMutex;

	/**
	 * Accept线程
	 */
	MainRunnable* mpMainRunnable;
	KThread* mpMainThread;

	/**
	 * 处理线程
	 */
	HandleRunnable* mpHandleRunnable;
	KThread** mpHandleThread;

	/**
	 * 处理线程数
	 */
	int miMaxThreadHandle;

	/**
	 * 是否运行
	 */
	bool mIsRunning;

	int mServer;
	int miMaxConnection;

	ev_io mAccept_watcher;
	ev_async mAsync_send_watcher;
	struct ev_loop *mLoop;

	TcpServerObserver *mpTcpServerObserver;

//	/**
//	 * 当前连接数
//	 */
//	int mCurrentConnection;
//	KMutex mConnectionMutex;

	/**
	 * 关闭socket队列
	 */
	CloseSocketQueue mCloseSocketQueue;
	KMutex mCloseSocketQueueMutex;

	/**
	 * 总接收包处理时间
	 */
	unsigned long mTotalHandleRecvTime;

	/**
	 * 总发送包处理时间
	 */
	unsigned long mTotalHandleSendTime;

	/**
	 * 总接收包时间
	 */
	unsigned long mTotalRecvTime;

	/**
	 * 总发送包时间
	 */
	unsigned long mTotalSendTime;

	/**
	 * 总时间
	 */
	unsigned long mTotalTime;
};

#endif /* TCPSERVER_H_ */
