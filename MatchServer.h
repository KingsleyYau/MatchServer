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
	void Run(
			short iPort,
			int mMaxClient,
			int iMaxMemoryCopy,
			int iMaxHandleThread,
			int iMaxQueryPerThread,
			const string& host,
			short dbPort,
			const string& dbName,
			const string& user,
			const string& passwd,
			int iMaxDatabaseThread,
			LOG_LEVEL iLogLevel,
			const string& logDir
			);
	void Reload();
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

	unsigned int GetTotal();
	unsigned int GetHit();
	unsigned int GetIgn();

private:

	TcpServer mClientTcpServer;
	RequestManager mRequestManager;
	DBManager mDBManager;

	TcpServer mClientTcpInsideServer;

	/**
	 * 是否运行
	 */
	bool mIsRunning;

	/**
	 * State线程
	 */
	StateRunnable* mpStateRunnable;
	KThread* mpStateThread;

	unsigned int mTotal;
	unsigned int mHit;
	unsigned int mIgn;

	string mConfigFile;
};

#endif /* MatchServer_H_ */
