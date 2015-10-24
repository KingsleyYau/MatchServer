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
	virtual ~TcpServerObserver(){};
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
	void SetHandleSize(unsigned int size);
	unsigned int GetHandleSize();

	/**
	 * deprecated
	 */
//	void SendMessage(Message *m);
	void SendMessageByQueue(Message *m);
	void SendMessageImmediately(Message *m);
	void SendAllMessageImmediately();

	void Disconnect(int fd);

	int GetSocket();
	MessageList* GetIdleMessageList();
	MessageList* GetHandleMessageList();
	MessageList* GetSendImmediatelyMessageList();

	/**
	 * deprecated
	 */
//	MessageList* GetSendMessageList();
//	MessageList* GetHandleSendMessageList();

	WatcherList *GetWatcherList();
	ev_io *GetAcceptWatcher();
	struct ev_loop *GetEvLoop();

	/**
	 * deprecated
	 */
	ev_async *GetAsyncSendWatcher();

	void Accept_Callback(ev_io *w, int revents);
	void Recv_Callback(ev_io *w, int revents);

	bool OnAccept(Message *m);
	void OnRecvMessage(Message *m);
	void OnSendMessage(Message *m);
	void OnDisconnect(int fd, Message *m);
	void OnTimeoutMessage(Message *m);
private:
	void StopEvio(ev_io *w);
	void CloseSocketIfNeedByHandleThread(int fd);

	void AddRecvTime(unsigned long time);
	void AddSendTime(unsigned long time);

	void LockWatcherList();
	void UnLockWatcherList();

	/* Thread safe message list */
	MessageList mIdleMessageList;
	MessageList mHandleMessageList;

	MessageList mSendImmediatelyMessageList;

	MessageList mHandleSendMessageList;
	MessageList* mpSendMessageList;

	/* Thread safe watcher list */
	WatcherList mWatcherList;
	KMutex mWatcherListMutex;

	int* mpHandlingMessageCount;
	bool* mpCloseRecv;
	KMutex mCloseMutex;

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

	/**
	 * 接收队列大小
	 */
	unsigned int mHandleSize;
};

#endif /* TCPSERVER_H_ */
