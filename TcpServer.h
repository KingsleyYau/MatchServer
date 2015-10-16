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

//typedef list<ev_io *> WatcherList;
typedef KSafeList<ev_io *> WatcherList;

class SendRunnable;
class HandleRunnable;
class MainRunnable;
class TcpServer;

class TcpServerObserver {
public:
	virtual bool OnAccept(TcpServer *ts, Message *m) = 0;
	virtual void OnRecvMessage(TcpServer *ts, Message *m) = 0;
	virtual void OnSendMessage(TcpServer *ts, Message *m) = 0;
	virtual void OnDisconnect(TcpServer *ts, int fd) = 0;
	virtual void OnTimeoutMessage(TcpServer *ts, Message *m) = 0;
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

	void SendMessageByQueue(Message *m);
	void SendMessageImmediately(Message *m);
	void SendAllMessageImmediately();

	void Disconnect(int fd);

	int GetSocket();
	MessageList* GetIdleMessageList();
	MessageList* GetHandleMessageList();
	MessageList* GetSendImmediatelyMessageList();

	MessageList* GetSendMessageList();
	MessageList* GetHandleSendMessageList();

	WatcherList *GetWatcherList();
	ev_async *GetAsyncSendWatcher();
	ev_io *GetAcceptWatcher();
	struct ev_loop *GetEvLoop();

	bool OnAccept(Message *m);
	void OnRecvMessage(Message *m);
	void OnSendMessage(Message *m);
	void OnDisconnect(int fd, Message *m);
	void OnTimeoutMessage(Message *m);

	void AddRecvTime(unsigned long time);
	void AddSendTime(unsigned long time);

//	void HandleCloseQueue();

	void LockWatcherList();
	void UnLockWatcherList();

	void SetHandleSize(unsigned int size);
	unsigned int GetHandleSize();

private:
	void StopEvio(ev_io *w);

	/* Thread safe message list */
	MessageList mIdleMessageList;
	MessageList mHandleMessageList;

	MessageList mSendImmediatelyMessageList;

	MessageList mHandleSendMessageList;
	MessageList* mpSendMessageList;

	/* Thread safe watcher list */
	WatcherList mWatcherList;
	KMutex mWatcherListMutex;

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
	HandleRunnable** mpHandleRunnable;
	KThread** mpHandleThread;

	/**
	 * 发送线程
	 */
	SendRunnable* mpSendRunnable;
	KThread* mpSendThread;

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
	 * 总接收包处理时间
	 */
	unsigned int mTotalHandleRecvTime;

	/**
	 * 总发送包处理时间
	 */
	unsigned int mTotalHandleSendTime;

	/**
	 * 总接收包时间
	 */
	unsigned int mTotalRecvTime;

	/**
	 * 总发送包时间
	 */
	unsigned int mTotalSendTime;

	/**
	 * 总时间
	 */
	unsigned int mTotalTime;

	unsigned int mHandleSize;
};

#endif /* TCPSERVER_H_ */
