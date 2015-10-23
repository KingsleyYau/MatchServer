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
#include "ConfFile.hpp"

class StateRunnable;
class MatchServer : public TcpServerObserver, RequestManagerCallback {
public:
	MatchServer();
	virtual ~MatchServer();

	void Run(const string& config);
	void Run();
	bool Reload();
	bool IsRunning();
	TcpServer* GetTcpServer();

	/* callback by TcpServerObserver */
	bool OnAccept(TcpServer *ts, Message *m);
	void OnRecvMessage(TcpServer *ts, Message *m);
	void OnSendMessage(TcpServer *ts, Message *m);
	void OnDisconnect(TcpServer *ts, int fd);
	void OnTimeoutMessage(TcpServer *ts, Message *m);

	/* callback by RequestManagerCallback */
	void OnReload(RequestManager* pRequestManager);

	void StateRunnableHandle();

private:
	TcpServer mClientTcpServer;
	TcpServer mClientTcpInsideServer;
	RequestManager mRequestManager;
	DBManager mDBManager;

	// BASE
	short miPort;
	int miMaxClient;
	int miMaxMemoryCopy;
	int miMaxHandleThread;
	int miMaxQueryPerThread;
	int miTimeout;

	// DB
	DBSTRUCT mDbQA;
	int miSyncTime;
	int miSyncOnlineTime;
	int miOnlineDbCount;
	DBSTRUCT mDbOnline[4];

	// LOG
	int miLogLevel;
	string mLogDir;
	int miDebugMode;

	/**
	 * 是否运行
	 */
	bool mIsRunning;

	/**
	 * State线程
	 */
	StateRunnable* mpStateRunnable;
	KThread* mpStateThread;

	/**
	 * 统计请求
	 */
	unsigned int mTotal;
	unsigned int mHit;
	unsigned int mResponed;
	KMutex mCountMutex;

	/**
	 * 配置文件
	 */
	string mConfigFile;

	/**
	 * 监听线程输出间隔
	 */
	unsigned int miStateTime;
};

#endif /* MatchServer_H_ */
