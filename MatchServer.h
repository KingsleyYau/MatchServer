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
#include "DBManager.h"
#include "ConfFile.hpp"
#include "json/json.h"

#include <map>
using namespace std;
typedef map<int, Message*> SyncMessageMap;

class StateRunnable;
class MatchServer : public TcpServerObserver, DBManagerListener {
public:
	MatchServer();
	virtual ~MatchServer();

	void Run(const string& config);
	void Run();
	bool Reload();
	bool IsRunning();

	/* callback by TcpServerObserver */
	bool OnAccept(TcpServer *ts, Message *m);
	void OnRecvMessage(TcpServer *ts, Message *m);
	void OnSendMessage(TcpServer *ts, Message *m);
	void OnDisconnect(TcpServer *ts, int fd);
	void OnTimeoutMessage(TcpServer *ts, Message *m);

	/* callback by DBManager */
	void OnSyncFinish(DBManager* pDBManager);

	void StateRunnableHandle();

private:
	/*
	 *	请求解析函数
	 *	return : -1:Send fail respond / 0:Continue recv, send no respond / 1:Send OK respond
	 */
	int HandleRecvMessage(Message *m, Message *sm);
	int HandleTimeoutMessage(Message *m, Message *sm);
	int HandleInsideRecvMessage(Message *m, Message *sm);

	/**
	 * 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
	 */
	bool QuerySameAnswerLadyList(
			Json::Value& womanListNode,
			const char* pManId,
			const char* pSiteId,
			const char* pLimit,
			Message *m
			);

	/**
	 * 2.获取跟男士有指定共同问题的女士Id列表接口(http get)
	 */
	bool QueryTheSameQuestionLadyList(
			Json::Value& womanListNode,
			const char* pQid,
			const char* pSiteId,
			const char* pLimit,
			Message *m
			);

	/**
	 * 3.获取跟男士有任意共同问题的女士Id列表接口(http get)
	 */
	bool QueryAnySameQuestionLadyList(
			Json::Value& womanListNode,
			const char* pManId,
			const char* pSiteId,
			const char* pLimit,
			Message *m
			);

	/**
	 * 4.获取回答过注册问题的在线女士Id列表接口(http get)
	 */
	bool QueryAnySameQuestionOnlineLadyList(
			Json::Value& womanListNode,
			const char* pQids,
			const char* pSiteId,
			const char* pLimit,
			Message *m
			);


	TcpServer mClientTcpServer;
	TcpServer mClientTcpInsideServer;
	DBManager mDBManager;

	/**
	 * 等待发送队列
	 */
	SyncMessageMap mWaitForSendMessageMap;
	KMutex mWaitForSendMessageMapMutex;

	// BASE
	short miPort;
	int miMaxClient;
	int miMaxMemoryCopy;
	int miMaxHandleThread;
	int miMaxQueryPerThread;
	/**
	 * 请求超时(秒)
	 */
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
