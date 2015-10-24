/*
 * MatchServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "MatchServer.h"
#include "DataHttpParser.h"
#include "TimeProc.hpp"

#include <sys/syscall.h>

/* state thread */
class StateRunnable : public KRunnable {
public:
	StateRunnable(MatchServer *container) {
		mContainer = container;
	}
	virtual ~StateRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->StateRunnableHandle();
	}
private:
	MatchServer *mContainer;
};

MatchServer::MatchServer() {
	// TODO Auto-generated constructor stub
	mpStateRunnable = new StateRunnable(this);
}

MatchServer::~MatchServer() {
	// TODO Auto-generated destructor stub
	if( mpStateRunnable ) {
		delete mpStateRunnable;
	}
}

void MatchServer::Run(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// Reload config
		if( Reload() ) {
			if( miDebugMode == 1 ) {
				LogManager::LogSetFlushBuffer(0);
			} else {
				LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
			}

			Run();
		} else {
			printf("# Match Server can not load config file exit. \n");
		}

	} else {
		printf("# No config file can be use exit. \n");
	}
}

void MatchServer::Run() {
	/* log system */
	LogManager::GetLogManager()->Start(1000, miLogLevel, mLogDir);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::Run( "
			"miPort : %d, "
			"miMaxClient : %d, "
			"miMaxMemoryCopy : %d, "
			"miMaxHandleThread : %d, "
			"miMaxQueryPerThread : %d, "
			"miTimeout : %d, "
			"miStateTime, %d, "
			"miLogLevel : %d, "
			"mlogDir : %s "
			")",
			miPort,
			miMaxClient,
			miMaxMemoryCopy,
			miMaxHandleThread,
			miMaxQueryPerThread,
			miTimeout,
			miStateTime,
			miLogLevel,
			mLogDir.c_str()
			);

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::Run( "
			"mDbQA.mHost : %s, "
			"mDbQA.mPort : %d, "
			"mDbQA.mDbName : %s, "
			"mDbQA.mUser : %s, "
			"mDbQA.mPasswd : %s, "
			"mDbQA.miMaxDatabaseThread : %d, "
			"miOnlineDbCount : %d "
			")",
			mDbQA.mHost.c_str(),
			mDbQA.mPort,
			mDbQA.mDbName.c_str(),
			mDbQA.mUser.c_str(),
			mDbQA.mPasswd.c_str(),
			mDbQA.miMaxDatabaseThread,
			miOnlineDbCount
			);

	for(int i = 0; i < miOnlineDbCount; i++) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::Run( "
				"mDbOnline[%d].mHost : %s, "
				"mDbOnline[%d].mPort : %d, "
				"mDbOnline[%d].mDbName : %s, "
				"mDbOnline[%d].mUser : %s, "
				"mDbOnline[%d].mPasswd : %s, "
				"mDbOnline[%d].miMaxDatabaseThread : %d, "
				"mDbOnline[%d].miSiteId : %d "
				")",
				i,
				mDbOnline[i].mHost.c_str(),
				i,
				mDbOnline[i].mPort,
				i,
				mDbOnline[i].mDbName.c_str(),
				i,
				mDbOnline[i].mUser.c_str(),
				i,
				mDbOnline[i].mPasswd.c_str(),
				i,
				mDbOnline[i].miMaxDatabaseThread,
				i,
				mDbOnline[i].miSiteId
				);
	}

	bool bFlag = false;

	mTotal = 0;
	mHit = 0;
	mResponed = 0;

	/* db manager */
	mDBManager.SetDBManagerListener(this);
	bFlag = mDBManager.Init(
			miMaxMemoryCopy,
			mDbQA,
			mDbOnline,
			miOnlineDbCount,
			false
			);

	if( !bFlag ) {
		printf("# Match Server can not initialize database exit. \n");
		LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( Database Init fail )");
		return;
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( Database Init OK )");

	/* inside server */
	mClientTcpInsideServer.SetTcpServerObserver(this);
	mClientTcpInsideServer.SetHandleSize(1000);
	mClientTcpInsideServer.Start(20, miPort + 1, 1);
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( Inside TcpServer Init OK )");

	/* match server */
	mClientTcpServer.SetTcpServerObserver(this);
	/**
	 * 预估相应时间,内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpServer.SetHandleSize(miMaxMemoryCopy * miTimeout * miMaxQueryPerThread);
	mClientTcpServer.Start(miMaxClient, miPort, miMaxHandleThread);
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( TcpServer Init OK )");

	mIsRunning = true;

	mpStateThread = new KThread(mpStateRunnable);
	if( mpStateThread->start() != 0 ) {
//		printf("# Create state thread ok \n");
	}

	printf("# MatchServer start OK. \n");
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( Init OK )");

	/* call server */
	while( true ) {
		/* do nothing here */
		sleep(5);
	}
}

bool MatchServer::Reload() {
	bool bFlag = false;
	mConfigMutex.lock();
	if( mConfigFile.length() > 0 ) {
		ConfFile conf;
		conf.InitConfFile(mConfigFile.c_str(), "");
		if ( conf.LoadConfFile() ) {
			// BASE
			miPort = atoi(conf.GetPrivate("BASE", "PORT", "9876").c_str());
			miMaxClient = atoi(conf.GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
			miMaxMemoryCopy = atoi(conf.GetPrivate("BASE", "MAXMEMORYCOPY", "1").c_str());
			miMaxHandleThread = atoi(conf.GetPrivate("BASE", "MAXHANDLETHREAD", "2").c_str());
			miMaxQueryPerThread = atoi(conf.GetPrivate("BASE", "MAXQUERYPERCOPY", "10").c_str());
			miTimeout = atoi(conf.GetPrivate("BASE", "TIMEOUT", "10").c_str());
			miStateTime = atoi(conf.GetPrivate("BASE", "STATETIME", "30").c_str());

			// DB
			mDbQA.mHost = conf.GetPrivate("DB", "DBHOST", "localhost");
			mDbQA.mPort = atoi(conf.GetPrivate("DB", "DBPORT", "3306").c_str());
			mDbQA.mDbName = conf.GetPrivate("DB", "DBNAME", "qpidnetwork");
			mDbQA.mUser = conf.GetPrivate("DB", "DBUSER", "root");
			mDbQA.mPasswd = conf.GetPrivate("DB", "DBPASS", "123456");
			mDbQA.miMaxDatabaseThread = atoi(conf.GetPrivate("DB", "MAXDATABASETHREAD", "4").c_str());

			miSyncTime = atoi(conf.GetPrivate("DB", "SYNCHRONIZETIME", "30").c_str());
			miSyncOnlineTime = atoi(conf.GetPrivate("DB", "SYNCHRONIZE_ONLINE_TIME", "1").c_str());
			miOnlineDbCount = atoi(conf.GetPrivate("DB", "ONLINE_DB_COUNT", "0").c_str());
			miOnlineDbCount = miOnlineDbCount > 4?4:miOnlineDbCount;
			miOnlineDbCount = miOnlineDbCount < 0?0:miOnlineDbCount;

			char domain[4] = {'\0'};
			for(int i = 0; i < miOnlineDbCount; i++) {
				sprintf(domain, "DB_ONLINE_%d", i);
				mDbOnline[i].mHost = conf.GetPrivate(domain, "DBHOST", "localhost");
				mDbOnline[i].mPort = atoi(conf.GetPrivate(domain, "DBPORT", "3306").c_str());
				mDbOnline[i].mDbName = conf.GetPrivate(domain, "DBNAME", "qpidnetwork_online");
				mDbOnline[i].mUser = conf.GetPrivate(domain, "DBUSER", "root");
				mDbOnline[i].mPasswd = conf.GetPrivate(domain, "DBPASS", "123456");
				mDbOnline[i].miMaxDatabaseThread = atoi(conf.GetPrivate(domain, "MAXDATABASETHREAD", "4").c_str());
				mDbOnline[i].miSiteId = atoi(conf.GetPrivate(domain, "SITEID", "-1").c_str());
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::Reload( "
						"mDbOnline[%d].mHost : %s, "
						"mDbOnline[%d].mPort : %d, "
						"mDbOnline[%d].mDbName : %s, "
						"mDbOnline[%d].mUser : %s, "
						"mDbOnline[%d].mPasswd : %s, "
						"mDbOnline[%d].miMaxDatabaseThread : %d, "
						"mDbOnline[i].miSiteId : %d "
						")",
						i,
						mDbOnline[i].mHost.c_str(),
						i,
						mDbOnline[i].mPort,
						i,
						mDbOnline[i].mDbName.c_str(),
						i,
						mDbOnline[i].mUser.c_str(),
						i,
						mDbOnline[i].mPasswd.c_str(),
						i,
						mDbOnline[i].miMaxDatabaseThread,
						i,
						mDbOnline[i].miSiteId
						);
			}

			// LOG
			miLogLevel = atoi(conf.GetPrivate("LOG", "LOGLEVEL", "5").c_str());
			mLogDir = conf.GetPrivate("LOG", "LOGDIR", "log");
			miDebugMode = atoi(conf.GetPrivate("LOG", "DEBUGMODE", "0").c_str());

			// Reload config
			mDBManager.SetSyncTime(miSyncTime * 60);
			mDBManager.SetSyncOnlineTime(miSyncOnlineTime * 60);

			mClientTcpServer.SetHandleSize(miMaxMemoryCopy * miTimeout * miMaxQueryPerThread);

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"MatchServer::Reload( "
					"miPort : %d, "
					"miMaxClient : %d, "
					"miMaxMemoryCopy : %d, "
					"miMaxHandleThread : %d, "
					"miMaxQueryPerThread : %d, "
					"miTimeout : %d, "
					"miStateTime, %d, "
					"miLogLevel : %d, "
					"mlogDir : %s "
					")",
					miPort,
					miMaxClient,
					miMaxMemoryCopy,
					miMaxHandleThread,
					miMaxQueryPerThread,
					miTimeout,
					miStateTime,
					miLogLevel,
					mLogDir.c_str()
					);

			bFlag = true;
		}
	}
	mConfigMutex.unlock();
	return bFlag;
}

bool MatchServer::IsRunning() {
	return mIsRunning;
}

/**
 * New request
 */
bool MatchServer::OnAccept(TcpServer *ts, Message *m) {
	return true;
}

void MatchServer::OnRecvMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);
	Message *sm = ts->GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		sm->fd = m->fd;
		sm->wr = m->wr;
		int ret = -1;

		if( &mClientTcpServer == ts ) {
			// 外部服务请求
			mCountMutex.lock();
			mTotal++;
			mCountMutex.unlock();
			ret = HandleRecvMessage(m, sm);
			if( 0 != ret ) {
				mCountMutex.lock();
				mResponed += sm->totaltime;
				if( ret == 1 ) {
					mHit++;
				}
				mCountMutex.unlock();
			}

		} else if( &mClientTcpInsideServer == ts ){
			// 内部服务请求
			ret = HandleInsideRecvMessage(m, sm);
		}

		if( ret != 0 ) {
			// Process finish, send respond
			ts->SendMessageByQueue(sm);
		}
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"MatchServer::OnRecvMessage( "
				"tid : %d, "
				"m->fd : [%d], "
				"No idle message can be use "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
		// 断开连接
		ts->Disconnect(m->fd);
	}
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"end "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);
}

void MatchServer::OnSendMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnSendMessage( tid : %d, m->fd : [%d], start )", (int)syscall(SYS_gettid), m->fd);
	// 发送成功，断开连接
	ts->Disconnect(m->fd);
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnSendMessage( tid : %d, m->fd : [%d], end )", (int)syscall(SYS_gettid), m->fd);
}

void MatchServer::OnTimeoutMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnTimeoutMessage( tid : %d, m->fd : [%d], start )", (int)syscall(SYS_gettid), m->fd);
	Message *sm = ts->GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		sm->fd = m->fd;
		sm->wr = m->wr;

		mCountMutex.lock();
		mTotal++;
		mResponed += sm->totaltime;
		mCountMutex.unlock();

		HandleTimeoutMessage(m, sm);
		// Process finish, send respond
		ts->SendMessageByQueue(sm);
	} else {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"MatchServer::OnTimeoutMessage( "
				"tid : %d, "
				"m->fd : [%d], "
				"No idle message can be use "
				")",
				(int)syscall(SYS_gettid),
				m->fd
				);
		// 断开连接
		ts->Disconnect(m->fd);
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnTimeoutMessage( tid : %d, m->fd : [%d], end )", (int)syscall(SYS_gettid), m->fd);
}

/**
 * OnDisconnect
 */
void MatchServer::OnDisconnect(TcpServer *ts, int fd) {
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnDisconnect( "
			"tid : %d, "
			"fd : [%d], "
			"start "
			")",
			(int)syscall(SYS_gettid),
			fd
			);
	if( ts == &mClientTcpInsideServer ) {
		mWaitForSendMessageMapMutex.lock();
		SyncMessageMap::iterator itr = mWaitForSendMessageMap.find(fd);
		if( itr != mWaitForSendMessageMap.end() ) {

			ts->GetIdleMessageList()->PushBack(itr->second);
			mWaitForSendMessageMap.erase(itr);
		}
		mWaitForSendMessageMapMutex.unlock();
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnDisconnect( "
			"tid : %d, "
			"fd : [%d], "
			"end "
			")",
			(int)syscall(SYS_gettid),
			fd
			);
}

/* callback by DBManager */
void MatchServer::OnSyncFinish(DBManager* pDBManager) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::OnSyncFinish( "
			"tid : %d "
			")",
			(int)syscall(SYS_gettid)
			);

	mWaitForSendMessageMapMutex.lock();
	for(SyncMessageMap::iterator itr = mWaitForSendMessageMap.begin(); itr != mWaitForSendMessageMap.end(); itr++){
		mClientTcpInsideServer.SendMessageByQueue(itr->second);
	}
	mWaitForSendMessageMap.clear();
	mWaitForSendMessageMapMutex.unlock();
}

void MatchServer::StateRunnableHandle() {
	unsigned int iCount = 0;

	unsigned int iTotal = 0;
	double iSecondTotal = 0;

	unsigned int iHit = 0;
	double iSecondHit = 0;

	double iResponed = 0;

	unsigned int iStateTime = miStateTime;

	while( IsRunning() ) {
		if ( iCount < iStateTime ) {
			iCount++;
		} else {
			iCount = 0;
			iSecondTotal = 0;
			iSecondHit = 0;
			iResponed = 0;

			mCountMutex.lock();
			iTotal = mTotal;
			iHit = mHit;

			if( iStateTime != 0 ) {
				iSecondTotal = 1.0 * iTotal / iStateTime;
				iSecondHit = 1.0 * iHit / iStateTime;
			}
			if( iTotal != 0 ) {
				iResponed = 1.0 * mResponed / iTotal;
			}

			mHit = 0;
			mTotal = 0;
			mResponed = 0;
			mCountMutex.unlock();

//			LogManager::GetLogManager()->Log(LOG_WARNING,
//					"MatchServer::StateRunnable( tid : %d, TcpServer::GetIdleMessageList() : %d )",
//					(int)syscall(SYS_gettid),
//					(MessageList*) mClientTcpServer.GetIdleMessageList()->Size()
//					);
			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( tid : %d, TcpServer::GetHandleMessageList() : %d )",
					(int)syscall(SYS_gettid),
					(MessageList*) mClientTcpServer.GetHandleMessageList()->Size()
					);
			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( tid : %d, TcpServer::GetSendImmediatelyMessageList() : %d )",
					(int)syscall(SYS_gettid),
					(MessageList*) mClientTcpServer.GetSendImmediatelyMessageList()->Size()
					);
//			LogManager::GetLogManager()->Log(LOG_WARNING,
//					"MatchServer::StateRunnable( tid : %d, TcpServer::GetWatcherList() : %d )",
//					(int)syscall(SYS_gettid),
//					(WatcherList*) mClientTcpServer.GetWatcherList()->Size()
//					);
			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( "
					"tid : %d, "
					"iTotal : %u, "
					"iHit : %u, "
					"iSecondTotal : %.1lf, "
					"iSecondHit : %.1lf, "
					"iResponed : %.1lf, "
					"iStateTime : %u "
					")",
					(int)syscall(SYS_gettid),
					iTotal,
					iHit,
					iSecondTotal,
					iSecondHit,
					iResponed,
					iStateTime
					);
			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( "
					"tid : %d, "
					"过去%u秒共收到%u个请求, "
					"成功处理%u个请求, "
					"平均收到%.1lf个/秒, "
					"平均处理%.1lf个/秒, "
					"平均响应时间%.1lf毫秒/个"
					")",
					(int)syscall(SYS_gettid),
					iStateTime,
					iTotal,
					iHit,
					iSecondTotal,
					iSecondHit,
					iResponed
					);

			iStateTime = miStateTime;
		}
		sleep(1);
	}
}

int MatchServer::HandleRecvMessage(Message *m, Message *sm) {
	int ret = -1;
	int code = 200;
	char reason[16] = {"OK"};
	string param;

	Json::FastWriter writer;
	Json::Value rootSend;

	if( m == NULL ) {
		return ret;
	}

	DataHttpParser dataHttpParser;
	if ( DiffGetTickCount(m->starttime, GetTickCount()) < miTimeout * 1000 ) {
		if( m->buffer != NULL ) {
			ret = dataHttpParser.ParseData(m->buffer);
		}
	} else {
		param = writer.write(rootSend);
	}

	if( ret == 1 ) {
		ret = -1;
		const char* pPath = dataHttpParser.GetPath();
		HttpType type = dataHttpParser.GetType();

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"MatchServer::HandleRecvMessage( "
				"tid : %d, "
				"m->fd: [%d], "
				"type : %d, "
				"pPath : %s "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				type,
				pPath
				);

		if( type == GET ) {
			if( strcmp(pPath, "/QUERY_SAME_ANSWER_LADY_LIST") == 0 ) {
				// 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
				const char* pManId = dataHttpParser.GetParam("MANID");
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pManId != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QuerySameAnswerLadyList(womanListNode, pManId, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

				param = writer.write(rootSend);

			} else if( strcmp(pPath, "/QUERY_THE_SAME_QUESTION_LADY_LIST") == 0 ) {
				// 2.获取跟男士有指定共同问题的女士Id列表接口(http get)
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pQId = dataHttpParser.GetParam("QID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pQId != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QueryTheSameQuestionLadyList(womanListNode, pQId, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

				param = writer.write(rootSend);

			} else if( strcmp(pPath, "/QUERY_ANY_SAME_QUESTION_LADY_LIST") == 0 ) {
				// 3.获取跟男士有任意共同问题的女士Id列表接口(http get)
				const char* pManId = dataHttpParser.GetParam("MANID");
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pManId != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QueryAnySameQuestionLadyList(womanListNode, pManId, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

				param = writer.write(rootSend);

			} else if( strcmp(pPath, "/QUERY_ANY_SAME_QUESTION_ONLINE_LADY_LIST") == 0 ) {
				// 4.获取回答过注册问题的在线女士Id列表接口(http get)
				const char* pQIds = dataHttpParser.GetParam("QIDS");
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pQIds != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QueryAnySameQuestionOnlineLadyList(womanListNode, pQIds, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

				param = writer.write(rootSend);

			} else {
				code = 404;
				sprintf(reason, "Not Found");
				param = "404 Not Found";
			}
		} else {
			code = 404;
			sprintf(reason, "Not Found");
			param = "404 Not Found";
		}
	}

	sm->totaltime = GetTickCount() - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::HandleRecvMessage( "
			"tid : %d, "
			"m->fd: [%d], "
			"iTotaltime : %u ms, "
			"ret : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			sm->totaltime,
			ret
			);

//	rootSend["fd"] = m->fd;
//	rootSend["iTotaltime"] = sm->totaltime;

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 %d %s\r\nContext-Length:%d\r\n\r\n%s",
			code,
			reason,
			(int)param.length(),
			param.c_str()
			);
	sm->len = strlen(sm->buffer);

	return ret;
}

int MatchServer::HandleTimeoutMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	unsigned int iHandleTime = 0;

	if( m == NULL ) {
		return ret;
	}

	iHandleTime = GetTickCount();

	DataHttpParser dataHttpParser;
	if( m->buffer != NULL ) {
		ret = dataHttpParser.ParseData(m->buffer);
	}

	if( ret == 1 ) {

	}

	iHandleTime = GetTickCount() - iHandleTime;
	sm->totaltime = GetTickCount() - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::HandleTimeoutMessage( "
			"tid : %d, "
			"m->fd: [%d], "
			"iHandleTime : %u ms, "
			"iTotaltime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iHandleTime,
			sm->totaltime
			);

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
			(int)param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}

int MatchServer::HandleInsideRecvMessage(Message *m, Message *sm) {
	int ret = -1;
	int code = 200;
	char reason[16] = {"OK"};
	string param;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	if( m == NULL ) {
		return ret;
	} else {
		param = writer.write(rootSend);
	}

	DataHttpParser dataHttpParser;
	if ( DiffGetTickCount(m->starttime, GetTickCount()) < miTimeout * 1000 ) {
		if( m->buffer != NULL ) {
			ret = dataHttpParser.ParseData(m->buffer);
		}
	}

	if( ret == 1 ) {
		ret = -1;
		const char* pPath = dataHttpParser.GetPath();
		HttpType type = dataHttpParser.GetType();

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::HandleInsideRecvMessage( "
				"tid : %d, "
				"m->fd: [%d], "
				"type : %d, "
				"pPath : %s, "
				"parse "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				type,
				pPath
				);

		if( type == GET ) {
			if( strcmp(pPath, "/SYNC") == 0 ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::HandleInsideRecvMessage( "
						"tid : %d, "
						"m->fd : [%d], "
						"start "
						")",
						(int)syscall(SYS_gettid),
						m->fd
						);

				mWaitForSendMessageMapMutex.lock();
				mWaitForSendMessageMap.insert(SyncMessageMap::value_type(m->fd, sm));
				mWaitForSendMessageMapMutex.unlock();

				mDBManager.SyncForce();

				rootSend["ret"] = 1;
				param = writer.write(rootSend);
				ret = 0;
			} else if( strcmp(pPath, "/RELOAD") == 0 ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::HandleInsideRecvMessage( "
						"tid : %d, "
						"m->fd : [%d], "
						"start "
						")",
						(int)syscall(SYS_gettid),
						m->fd
						);
				if( Reload()) {
					rootSend["ret"] = 1;
				} else {
					rootSend["ret"] = 0;
				}
				param = writer.write(rootSend);
				ret = 1;
			} else {
				code = 404;
				sprintf(reason, "Not Found");
				param = "404 Not Found";
			}
		} else {
			code = 404;
			sprintf(reason, "Not Found");
			param = "404 Not Found";
		}

	}

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 %d %s\r\nContext-Length:%d\r\n\r\n%s",
			code,
			reason,
			(int)param.length(),
			param.c_str()
			);
	sm->len = strlen(sm->buffer);

	return ret;
}

/**
 * 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
 */
bool MatchServer::QuerySameAnswerLadyList(
		Json::Value& womanListNode,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pManId,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};
	sprintf(sql, "SELECT qid, aid FROM mq_man_answer_%s WHERE manid = '%s';", pSiteId, pManId);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iRow,
			iColumn,
			iQueryIndex
			);

	if( bResult && result && iRow > 0 ) {
		// 随机起始男士问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;

		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}
		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND aid = %s AND question_status = 1;",
						pSiteId,
						result[i * iColumn],
						result[i * iColumn + 1]
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"MatchServer::QuerySameAnswerLadyList( "
									"tid : %d, "
									"m->fd: [%d], "
									"Count iQueryTime : %u m, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
									m->fd,
									iQueryTime,
									iQueryIndex,
									iNum,
									iMax
									);

				/*
				 * 查询当前问题相同答案的女士Id集合
				 * 1.超过iMax个, 随机选取iMax个
				 * 2.不够iMax个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= iMax ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = iMax;
					iLadyIndex = (rand() % (iNum -iLadyCount));
				}

				sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
						"WHERE qid = %s AND aid = %s AND question_status = 1 "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						result[i * iColumn],
						result[i * iColumn + 1],
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"MatchServer::QuerySameAnswerLadyList( "
									"tid : %d, "
									"m->fd: [%d], "
									"Query iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iRow2 : %d, "
									"iColumn2 : %d, "
									"iLadyCount : %d, "
									"iLadyIndex : %d, "
									"womanidMap.size() : %d "
									")",
									(int)syscall(SYS_gettid),
									m->fd,
									iQueryTime,
									iQueryIndex,
									iRow2,
									iColumn2,
									iLadyCount,
									iLadyIndex,
									womanidMap.size()
									);


				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( (int)womanidMap.size() < iMax ) {
							// 结果集合还不够iMax个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够iMax个女士
					if( (int)womanidMap.size() >= iMax ) {
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QuerySameAnswerLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iMax
								);
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			// 对已经获取到的女士统计相同答案问题数量
			iQueryTime = GetTickCount();
			for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
				char** result3 = NULL;
				int iRow3;
				int iColumn3;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE womanid = '%s' AND qid = %s AND aid = %s AND question_status = 1;",
						pSiteId,
						itr->first.c_str(),
						result[i * iColumn],
						result[i * iColumn + 1]
						);

				bResult = mDBManager.Query(sql, &result3, &iRow3, &iColumn3, iQueryIndex);
				if( bResult && result3 && iRow3 > 0 ) {
					itr->second += atoi(result3[1 * iColumn3]);
				}
				mDBManager.FinishQuery(result3);
			}

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			iQueryTime = GetTickCount() - iQueryTime;
			LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QuerySameAnswerLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"Double Check iQueryTime : %u ms, "
								"iQueryIndex : %d, "
								"iSingleQueryTime : %u ms "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iQueryTime,
								iQueryIndex,
								iSingleQueryTime
								);

			i++;
			i = ((i - 1) % iRow) + 1;

			// 单词请求是否大于30ms
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}
		}

		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
			Json::Value womanNode;
			womanNode[itr->first] = itr->second;
			womanListNode.append(womanNode);
		}
	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

/**
 * 2.获取跟男士有指定共同问题的女士Id列表接口(http get)
 */
bool MatchServer::QueryTheSameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pQid,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"qid : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pQid,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};

	string qid = pQid;
	if(qid.length() > 2) {
		qid = qid.substr(2, qid.length() - 2);
	}

	sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
			"WHERE qid = %s;",
			pSiteId,
			qid.c_str()
			);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iRow,
			iColumn,
			iQueryIndex
			);

	if( bResult && result && iRow > 0 ) {
		char** result2 = NULL;
		int iRow2;
		int iColumn2;

		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		iQueryTime = GetTickCount();
		bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
		if( bResult && result2 && iRow > 0 ) {
			iNum = atoi(result2[1 * iColumn2]);
		}
		mDBManager.FinishQuery(result2);
		iQueryTime = GetTickCount() - iQueryTime;

		LogManager::GetLogManager()->Log(
							LOG_STAT,
							"MatchServer::QueryTheSameQuestionLadyList( "
							"tid : %d, "
							"m->fd: [%d], "
							"Count iQueryTime : %u ms, "
							"iQueryIndex : %d, "
							"iNum : %d, "
							"iMax : %d "
							")",
							(int)syscall(SYS_gettid),
							m->fd,
							iQueryTime,
							iQueryIndex,
							iNum,
							iMax
							);

		/*
		 * 查询当前问题相同答案的女士Id集合
		 * 1.超过iMax个, 随机选取iMax个
		 * 2.不够iMax个, 全部选择
		 */
		int iLadyIndex = 0;
		int iLadyCount = 0;
		if( iNum <= iMax ) {
			iLadyCount = iNum;
		} else {
			iLadyCount = iMax;
			iLadyIndex = (rand() % (iNum -iLadyCount));
		}

		sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
				"WHERE qid = %s "
				"LIMIT %d OFFSET %d;",
				pSiteId,
				qid.c_str(),
				iLadyCount,
				iLadyIndex
				);

		iQueryTime = GetTickCount();
		bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
		iQueryTime = GetTickCount() - iQueryTime;

		if( bResult && result2 && iRow2 > 0 ) {
			for( int j = 1; j < iRow2 + 1; j++ ) {
				// insert womanid
				womanid = result2[j * iColumn2];
				womanListNode.append(womanid);
			}
		}

		mDBManager.FinishQuery(result2);

		LogManager::GetLogManager()->Log(
							LOG_STAT,
							"MatchServer::QueryTheSameQuestionLadyList( "
							"tid : %d, "
							"m->fd: [%d], "
							"Query iQueryTime : %u ms, "
							"iQueryIndex : %d, "
							"iRow2 : %d, "
							"iColumn2 : %d, "
							"iLadyCount : %d, "
							"iLadyIndex : %d "
							")",
							(int)syscall(SYS_gettid),
							m->fd,
							iQueryTime,
							iQueryIndex,
							iRow2,
							iColumn2,
							iLadyCount,
							iLadyIndex
							);


	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	usleep(1000 * iSingleQueryTime);
	iHandleTime = GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iHandleTime
			);

	return bResult;
}

/**
 * 3.获取跟男士有任意共同问题的女士Id列表接口(http get)
 */
bool MatchServer::QueryAnySameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pManId,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};
	sprintf(sql, "SELECT qid FROM mq_man_answer_%s "
			"WHERE manid = '%s';",
			pSiteId,
			pManId
			);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iRow,
			iColumn,
			iQueryIndex
			);

	if( bResult && result && iRow > 0 ) {
		// 随机起始男士问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;

		int iNum = 0;

		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND question_status = 1;",
						pSiteId,
						result[i * iColumn]
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"MatchServer::QueryAnySameQuestionLadyList( "
						"tid : %d, "
						"m->fd: [%d], "
						"Count iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iNum : %d, "
						"iMax : %d "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						iQueryTime,
						iQueryIndex,
						iNum,
						iMax
						);

				/*
				 * 查询当前问题相同的女士Id集合
				 * 1.超过iMax个, 随机选取iMax个
				 * 2.不够iMax个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= iMax ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = iMax;
					iLadyIndex = (rand() % (iNum -iLadyCount));
				}

				sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
						"WHERE qid = %s AND question_status = 1 "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						result[i * iColumn],
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"MatchServer::QueryAnySameQuestionLadyList( "
						"tid : %d, "
						"m->fd: [%d], "
						"Query iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iRow2 : %d, "
						"iColumn2 : %d, "
						"iLadyCount : %d, "
						"iLadyIndex : %d, "
						"womanidMap.size() : %d "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						iQueryTime,
						iQueryIndex,
						iRow2,
						iColumn2,
						iLadyCount,
						iLadyIndex,
						womanidMap.size()
						);

				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( (int)womanidMap.size() < iMax ) {
							// 结果集合还不够iMax个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够iMax个女士
					if( (int)womanidMap.size() >= iMax ) {
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QueryAnySameQuestionLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iMax
								);
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			i++;
			i = ((i - 1) % iRow) + 1;

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}
		}

		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
			womanListNode.append(itr->first);
		}
	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

/**
 * 4.获取回答过注册问题的在线女士Id列表接口(http get)
 */
bool MatchServer::QueryAnySameQuestionOnlineLadyList(
		Json::Value& womanListNode,
		const char* pQids,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryAnySameQuestionOnlineLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"qids : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pQids,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	char* pQid = NULL;
	char *pFirst = NULL;

	bool bResult = false;
	int iCount = 0;
	string qid;

	if( pQids != NULL ) {
		bool bEnougthLady = false;

		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		pQid = strtok_r((char*)pQids, ",", &pFirst);
		while( pQid != NULL && iCount < 7 ) {
			iSingleQueryTime = GetTickCount();

			qid = pQid;
			if(qid.length() > 2) {
				qid = qid.substr(2, qid.length() - 2);
			}

			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM online_woman_%s as o "
						"JOIN mq_woman_answer_%s as m "
						"ON o.womanid = m.womanid "
						"WHERE m.qid = %s"
						";",
						pSiteId,
						pSiteId,
						qid.c_str()
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"MatchServer::QueryAnySameQuestionOnlineLadyList( "
									"tid : %d, "
									"m->fd: [%d], "
									"Count iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
									m->fd,
									iQueryTime,
									iQueryIndex,
									iNum,
									iMax
									);

				/*
				 * 查询当前问题相同的女士Id集合
				 * 1.超过iMax个, 随机选取iMax个
				 * 2.不够iMax个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= iMax ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = iMax;
					iLadyIndex = (rand() % (iNum -iLadyCount));
				}

				sprintf(sql, "SELECT m.womanid FROM online_woman_%s as o "
						"JOIN mq_woman_answer_%s as m "
						"ON o.womanid = m.womanid "
						"WHERE m.qid = %s "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						pSiteId,
						qid.c_str(),
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"MatchServer::QueryAnySameQuestionOnlineLadyList( "
						"tid : %d, "
						"m->fd: [%d], "
						"Query iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iRow2 : %d, "
						"iColumn2 : %d, "
						"iLadyCount : %d, "
						"iLadyIndex : %d, "
						"womanidMap.size() : %d "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						iQueryTime,
						iQueryIndex,
						iRow2,
						iColumn2,
						iLadyCount,
						iLadyIndex,
						womanidMap.size()
						);

				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < 30 ) {
							// 结果集合还不够30个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够iMax个女士
					if( (int)womanidMap.size() >= iMax ) {
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QueryAnySameQuestionOnlineLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iMax
								);
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			iCount++;

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}

			pQid = strtok_r(NULL, ",", &pFirst);
		}

		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
			womanListNode.append(itr->first);
		}
	}

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::QueryAnySameQuestionOnlineLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}
