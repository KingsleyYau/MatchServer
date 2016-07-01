/*
 * MatchServer.h
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef MatchServer_H_
#define MatchServer_H_

#define VERSION_STRING "1.0.3"

#include "Client.h"
#include "MessageList.h"
#include "TcpServer.h"
#include "DBManager.h"

#include <common/ConfFile.hpp>
#include <common/KSafeMap.h>
#include <common/KSafeList.h>
#include <common/TimeProc.hpp>
#include <common/StringHandle.h>
#include <json/json/json.h>

using namespace std;

typedef KSafeMap<int, Client*> ClientSyncMessageMap;

// socket -> client
typedef KSafeMap<int, Client*> ClientMap;

class StateRunnable;
class MatchServer : public TcpServerObserver, DBManagerListener, ClientCallback {
public:
	MatchServer();
	virtual ~MatchServer();

	bool Run(const string& config);
	bool Run();
	bool Reload();
	bool IsRunning();

	/**
	 * TcpServer回调
	 */
	bool OnAccept(TcpServer *ts, int fd, char* ip);
	void OnRecvMessage(TcpServer *ts, Message *m);
	void OnSendMessage(TcpServer *ts, Message *m);
	void OnDisconnect(TcpServer *ts, int fd);
	void OnClose(TcpServer *ts, int fd);
	void OnTimeoutMessage(TcpServer *ts, Message *m);

	/**
	 * 内部服务(HTTP), 命令回调
	 */
	void OnClientQuerySameAnswerLadyList(Client* client, const char* pManId, const char* pSiteId,	const char* pLimit, long long iRegTime);
	void OnClientQueryTheSameQuestionLadyList(Client* client, const char* pQId, const char* pSiteId, const char* pLimit);
	void OnClientQueryAnySameQuestionLadyList(Client* client, const char* pManId,	const char* pSiteId, const char* pLimit);
	void OnClientQuerySameQuestionOnlineLadyList(Client* client, const char* pQIds, const char* pSiteId, const char* pLimit);
	void OnClientQuerySameQuestionAnswerLadyList(Client* client, const char* pQIds, const char* pAIds, const char* pSiteId, const char* pLimit);
	void OnClientQuerySameQuestionLadyList(Client* client, const char* pQIds, const char* pSiteId, const char* pLimit);
	void OnClientSync(Client* client);
	void OnClientUndefinedCommand(Client* client);

	/**
	 * 内部服务(HTTP), 发送请求响应
	 * @return true:发送成功/false:发送失败
	 */
	bool SendRespond2Client(
			TcpServer *ts,
			Client* client,
			Json::Value& root,
			bool success = true,
			bool found = true
			);

	/* 数据库处理回调 */
	void OnSyncFinish(DBManager* pDBManager);

	/**
	 * 增加返回记录
	 */
	void AddResopnd(bool hit, long long respondTime);

	/**
	 * 增加处理时间
	 */
	void AddHandleTime(long long time);

	/**
	 * 统计线程
	 */
	void StateRunnableHandle();

private:
	void TcpServerRecvMessageHandle(TcpServer *ts, Message *m);
	void TcpServerTimeoutMessageHandle(TcpServer *ts, Message *m);

	void TcpServerPublicRecvMessageHandle(TcpServer *ts, Message *m);
	void TcpServerPublicTimeoutMessageHandle(TcpServer *ts, Message *m);

	/***************************** 内部服务接口 **************************************/
	/**
	 * 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
	 * @param pManId	男士Id
	 * @param pSiteId	当前站点Id
	 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
	 * @param iRegTime 	男士注册时间(UNIX TIMESTAMP)
	 */
	bool QuerySameAnswerLadyList(
			Json::Value& womanListNode,
			const char* pManId,
			const char* pSiteId,
			const char* pLimit,
			long long iRegTime,
			int iQueryIndex
			);

	/**
	 * 2.获取有指定共同问题的女士Id列表接口(http get)
	 * @param pQid		问题Id
	 * @param pSiteId	当前站点Id
	 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
	 */
	bool QueryTheSameQuestionLadyList(
			Json::Value& womanListNode,
			const char* pQid,
			const char* pSiteId,
			const char* pLimit,
			int iQueryIndex
			);

	/**
	 * 3.获取有任意共同问题的女士Id列表接口(http get)
	 * @param pManId	男士Id
	 * @param pSiteId	当前站点Id
	 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
	 */
	bool QueryAnySameQuestionLadyList(
			Json::Value& womanListNode,
			const char* pManId,
			const char* pSiteId,
			const char* pLimit,
			int iQueryIndex
			);

	/**
	 * 4.获取回答过注册问题的在线女士Id列表接口(http get)
	 * @param pQids		问题Id列表(逗号隔开, 最多:7个)
	 * @param pSiteId	当前站点Id
	 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
	 */
	bool QuerySameQuestionOnlineLadyList(
			Json::Value& womanListNode,
			const char* pQids,
			const char* pSiteId,
			const char* pLimit,
			int iQueryIndex
			);

	/**
	 * 5.获取回答过注册问题有共同答案的女士Id列表接口(http get)
	 * @param pQids		问题Id列表(逗号隔开, 最多:7个)
	 * @param pAids		答案Id列表(逗号隔开, 最多:7个)
	 * @param pSiteId	当前站点Id
	 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
	 */
	bool QuerySameQuestionAnswerLadyList(
			Json::Value& womanListNode,
			const char* pQids,
			const char* pAids,
			const char* pSiteId,
			const char* pLimit,
			int iQueryIndex
			);

	/**
	 * 6.获取回答过注册问题有共同答案的女士Id列表接口(http get)
	 * @param pQids		问题Id列表(逗号隔开, 最多:7个)
	 * @param pSiteId	当前站点Id
	 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
	 */
	bool QuerySameQuestionLadyList(
			Json::Value& womanListNode,
			const char* pQids,
			const char* pSiteId,
			const char* pLimit,
			int iQueryIndex
			);
	/***************************** 内部服务接口 end **************************************/

	/***************************** 基本参数 **************************************/
	/**
	 * 监听端口
	 */
	short miPort;

	/**
	 * 最大连接数
	 */
	int miMaxClient;

	/**
	 * 处理线程数目
	 */
	int miMaxHandleThread;

	/**
	 * 内存copy数目
	 */
	int miMaxMemoryCopy;

	/**
	 * 每条线程处理任务速度(个/秒)
	 */
	int miMaxQueryPerThread;

	/**
	 * 请求超时(秒)
	 */
	unsigned int miTimeout;
	/***************************** 基本参数 end **************************************/

	/***************************** 数据库参数 **************************************/
	/**
	 * 问题库连接配置
	 */
	DBSTRUCT mDbQA;
	int miSyncTime;
	int miSyncOnlineTime;

	/**
	 * 在线库连接配置
	 */
	int miOnlineDbCount;
	DBSTRUCT mDbOnline[4];
	/***************************** 数据库参数 end **************************************/

	/***************************** 日志参数 **************************************/
	/**
	 * 日志等级
	 */
	int miLogLevel;

	/**
	 * 日志路径
	 */
	string mLogDir;

	/**
	 * 是否debug模式
	 */
	int miDebugMode;
	/***************************** 日志参数 end **************************************/

	/***************************** 统计参数 **************************************/
	/**
	 * 统计请求总数
	 */
	unsigned int mTotal;
	/**
	 * 统计成功处理请求总数
	 */
	unsigned int mHit;
	/**
	 * 统计响应总时间
	 */
	long long mResponedTime;
	/**
	 * 统计处理总时间
	 */
	long long mHandleTime;
	KMutex mCountMutex;

	/**
	 * 监听线程输出间隔
	 */
	unsigned int miStateTime;

	/***************************** 统计参数 end **************************************/

	/***************************** 运行参数 **************************************/
	/**
	 * 是否运行
	 */
	bool mIsRunning;

	/**
	 * 内部服务(HTTP)
	 */
	TcpServer mTcpServer;
	/**
	 * 外部服务(HTTP)
	 */
	TcpServer mTcpServerPublic;

	/**
	 * 客户端缓存数据包buffer
	 */
	MessageList mIdleMessageList;

	/**
	 * 数据库管理
	 */
	DBManager mDBManager;

	/**
	 * 配置文件锁
	 */
	KMutex mConfigMutex;

	/**
	 * State线程
	 */
	StateRunnable* mpStateRunnable;
	KThread* mpStateThread;

	/**
	 * 配置文件
	 */
	string mConfigFile;

	/*
	 * 内部服务(HTTP), 在线客户端列表
	 */
	ClientMap mClientMap;

	/***************************** 运行参数 end **************************************/
};

#endif /* MatchServer_H_ */
