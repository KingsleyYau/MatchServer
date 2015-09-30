/*
 * MatchServer.h
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef MatchServer_H_
#define MatchServer_H_

#include "MessageList.h"
#include "TcpServer.h"
#include "RequestManager.h"
#include "DBManager.h"

class StateRunnable;
class MatchServer : public TcpServerObserver {
public:
	MatchServer();
	virtual ~MatchServer();

	void Run(int mMaxClient, int iMaxMemoryCopy = 1, int iMaxHandleThread = 1);
	bool IsRunning();
	TcpServer* GetTcpServer();

	/* callback by TcpServerObserver */
	bool OnAccept(TcpServer *ts, Message *m);
	void OnRecvMessage(TcpServer *ts, Message *m);
	void OnSendMessage(TcpServer *ts, Message *m);
	void OnDisconnect(TcpServer *ts, int fd);

private:
	unsigned int GetTickCount();
	TcpServer mClientTcpServer;
	RequestManager mRequestManager;
	DBManager mDBManager;

	int miMaxClient;
	int miMaxMemoryCopy;

	/**
	 * 是否运行
	 */
	bool mIsRunning;

	/**
	 * State线程
	 */
	StateRunnable* mpStateRunnable;
	KThread* mpStateThread;
};

#endif /* MatchServer_H_ */
