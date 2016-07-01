/*
 * Client.h
 *
 *  Created on: 2015-11-10
 *      Author: Max
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "LogManager.h"
#include "MessageList.h"
#include "DataHttpParser.h"

#include <common/Arithmetic.hpp>
#include <common/Buffer.h>
#include <common/KCond.h>
#include <common/KSafeMap.h>

#include <string>

using namespace std;

class Client;
class ClientCallback {
public:
	virtual ~ClientCallback(){};

	/**
	 * 1.获取跟男士有任意共同答案的问题的女士Id列表接口
	 */
	virtual void OnClientQuerySameAnswerLadyList(
			Client* client,
			const char* pManId,
			const char* pSiteId,
			const char* pLimit,
			long long iRegTime
			) = 0;

	/**
	 * 2.获取有指定共同问题的女士Id列表接口
	 */
	virtual void OnClientQueryTheSameQuestionLadyList(
			Client* client,
			const char* pQId,
			const char* pSiteId,
			const char* pLimit
			) = 0;

	/**
	 * 3.获取有任意共同问题的女士Id列表接口
	 */
	virtual void OnClientQueryAnySameQuestionLadyList(
			Client* client,
			const char* pManId,
			const char* pSiteId,
			const char* pLimit
			) = 0;

	/**
	 * 4.获取回答过注册问题的在线女士Id列表接口
	 */
	virtual void OnClientQuerySameQuestionOnlineLadyList(
			Client* client,
			const char* pQIds,
			const char* pSiteId,
			const char* pLimit
			) = 0;

	/**
	 * 5.获取指定问题有共同答案的女士Id列表接口
	 */
	virtual void OnClientQuerySameQuestionAnswerLadyList(
			Client* client,
			const char* pQIds,
			const char* pAIds,
			const char* pSiteId,
			const char* pLimit
			) = 0;

	/**
	 * 6.获取回答过注册问题的女士Id列表接口
	 */
	virtual void OnClientQuerySameQuestionLadyList(
			Client* client,
			const char* pQIds,
			const char* pSiteId,
			const char* pLimit
			) = 0;

	/**
	 * 外部接口, 同步数据库
	 */
	virtual void OnClientSync(Client* client) = 0;

	/**
	 * 未定义的接口
	 */
	virtual void OnClientUndefinedCommand(Client* client) = 0;
};

typedef KSafeMap<int, Message*> MessageMap;
class Client {
public:
	Client();
	virtual ~Client();

	/**
	 * 空闲数据包队列
	 * 缓存的数据包列表
	 */
	void SetClientCallback(ClientCallback* pClientCallback);
	void SetMessageList(MessageList*);

	/**
	 * 解析数据
	 */
	int ParseData(Message* m);

	/**
	 * 增加发送中协议包
	 */
	int AddSendingPacket();

	/**
	 * 增加完成包
	 */
	int AddSentPacket();

	/**
	 * 是否最后一个发送的协议包序号
	 */
	bool IsAllPacketSent();

	int fd;
	bool isOnline;
    string ip;
	int	index;
	unsigned int 	starttime;

private:
    /**
     * 解析当前数据包
     */
    int ParseCurrentPackage(Message* m);

    /**
     * 根据http内容处理任务
     */
    void ParseCommand(DataHttpParser* pDataHttpParser);

    /**
     * 组包成功回调
     */
    ClientCallback* mpClientCallback;

    /**
     * 多线程组包/解包互斥锁
     */
    KMutex mKMutex;

    /**
     * 未组成协议包的临时buffer
     */
    Buffer mBuffer;

    /**
     * 多线程发包统计互斥锁
     */
    KMutex mKMutexSend;

    /**
     * 发送中协议包数量
     */
    int miSendingPacket;

    /**
     * 发送完成协议包数量
     */
    int miSentPacket;

    /**
     * 需要接收的数据包序列号
     */
    int miDataPacketRecvSeq;

    /**
     * 空闲数据包队列
     */
	MessageList* mpIdleMessageList;

	/**
	 * 缓存的数据包列表
	 */
	MessageMap mMessageMap;

	/**
	 * 已经解析错误标记
	 */
	bool mbError;
};

#endif /* CLIENT_H_ */
