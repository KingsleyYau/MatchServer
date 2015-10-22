/*
 * MatchServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "MatchServer.h"
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
	bool bFlag = false;

	mTotal = 0;
	mHit = 0;
	mResponed = 0;

	/* db manager */
	bFlag = mDBManager.Init(miMaxMemoryCopy, false, 4);
	bFlag = bFlag && mDBManager.InitSyncDataBase(
			mDbQA,
			mDbOnline,
			4
			);

	if( !bFlag ) {
		printf("# Match Server can not initialize database exit. \n");
		LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( Database Init fail )");
		return;
	}
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( Database Init OK )");

	/* request manager */
	mRequestManager.Init(&mDBManager);
	mRequestManager.SetRequestManagerCallback(this);
	mRequestManager.SetTimeout(miTimeout * 1000);
	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::Run( RequestManager Init OK )");

	/* inside server */
	mClientTcpInsideServer.SetTcpServerObserver(this);
	mClientTcpInsideServer.SetHandleSize(1000);
	mClientTcpInsideServer.Start(10, miPort + 1, 2);
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
	if( mConfigFile.length() > 0 ) {
		ConfFile* conf = new ConfFile();
		conf->InitConfFile(mConfigFile.c_str(), "");
		if ( !conf->LoadConfFile() ) {
			delete conf;
			return false;
		}

		// BASE
		miPort = atoi(conf->GetPrivate("BASE", "PORT", "9876").c_str());
		miMaxClient = atoi(conf->GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
		miMaxMemoryCopy = atoi(conf->GetPrivate("BASE", "MAXMEMORYCOPY", "1").c_str());
		miMaxHandleThread = atoi(conf->GetPrivate("BASE", "MAXHANDLETHREAD", "2").c_str());
		miMaxQueryPerThread = atoi(conf->GetPrivate("BASE", "MAXQUERYPERCOPY", "10").c_str());
		miTimeout = atoi(conf->GetPrivate("BASE", "TIMEOUT", "10").c_str());
		miStateTime = atoi(conf->GetPrivate("BASE", "STATETIME", "30").c_str());

		// DB
		mDbQA.mHost = conf->GetPrivate("DB", "DBHOST", "localhost");
		mDbQA.mPort = atoi(conf->GetPrivate("DB", "DBPORT", "3306").c_str());
		mDbQA.mDbName = conf->GetPrivate("DB", "DBNAME", "qpidnetwork");
		mDbQA.mUser = conf->GetPrivate("DB", "DBUSER", "root");
		mDbQA.mPasswd = conf->GetPrivate("DB", "DBPASS", "123456");
		mDbQA.miMaxDatabaseThread = atoi(conf->GetPrivate("DB", "MAXDATABASETHREAD", "4").c_str());
		miSyncTime = atoi(conf->GetPrivate("DB", "SYNCHRONIZETIME", "30").c_str());
		miSyncOnlineTime = atoi(conf->GetPrivate("DB", "SYNCHRONIZE_ONLINE_TIME", "1").c_str());

		char domain[8] = {'\0'};
		for(int i = 0; i < 4; i++) {
			sprintf(domain, "DB_%d", i + 1);
			mDbOnline[i].mHost = conf->GetPrivate(domain, "DBHOST", "localhost");
			mDbOnline[i].mPort = atoi(conf->GetPrivate(domain, "DBPORT", "3306").c_str());
			mDbOnline[i].mDbName = conf->GetPrivate(domain, "DBNAME", "qpidnetwork_online");
			mDbOnline[i].mUser = conf->GetPrivate(domain, "DBUSER", "root");
			mDbOnline[i].mPasswd = conf->GetPrivate(domain, "DBPASS", "123456");
			mDbOnline[i].miMaxDatabaseThread = atoi(conf->GetPrivate(domain, "MAXDATABASETHREAD", "4").c_str());
		}

		// LOG
		miLogLevel = atoi(conf->GetPrivate("LOG", "LOGLEVEL", "5").c_str());
		mLogDir = conf->GetPrivate("LOG", "LOGDIR", "log");
		miDebugMode = atoi(conf->GetPrivate("LOG", "DEBUGMODE", "0").c_str());

		// Reload config
		mDBManager.SetSyncTime(miSyncTime * 60);
		mDBManager.SetSyncOnlineTime(miSyncOnlineTime * 60);

		mRequestManager.SetTimeout(miTimeout * 1000);
		mClientTcpServer.SetHandleSize(miMaxMemoryCopy * miTimeout * miMaxQueryPerThread);

		delete conf;

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

		return true;
	}
	return false;
}

bool MatchServer::IsRunning() {
	return mIsRunning;
}

TcpServer* MatchServer::GetTcpServer() {
	return &mClientTcpServer;
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
		int ret;

		if( &mClientTcpServer == ts ) {
			mCountMutex.lock();
			mTotal++;
			mCountMutex.unlock();
			ret = mRequestManager.HandleRecvMessage(m, sm);
			if( 0 != ret ) {
				mCountMutex.lock();
				mResponed += sm->totaltime;
				if( ret == 1 ) {
					mHit++;
				}
				mCountMutex.unlock();
			}
		} else if( &mClientTcpInsideServer == ts ){
			ret = mRequestManager.HandleInsideRecvMessage(m, sm);
		}

		// Process finish, send respond
		ts->SendMessageByQueue(sm);
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
		mCountMutex.unlock();

		mRequestManager.HandleTimeoutMessage(m, sm);
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
//	LogManager::GetLogManager()->Log(LOG_STAT, "MatchServer::OnDisconnect( tid: %d, fd : [%d] )", (int)syscall(SYS_gettid), fd);
}

void MatchServer::OnReload(RequestManager* pRequestManager) {
	Reload();
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

			mCountMutex.lock();
			iSecondTotal = 1.0 * (mTotal - iTotal) / iStateTime;
			iSecondHit = 1.0 * (mHit - iHit) / iStateTime;
			iTotal = mTotal;
			iHit = mHit;
			iResponed = 1.0 * mResponed / iHit;
			mResponed = 0;
			mCountMutex.unlock();

			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( tid : %d, TcpServer::GetIdleMessageList() : %d )",
					(int)syscall(SYS_gettid),
					(MessageList*) GetTcpServer()->GetIdleMessageList()->Size()
					);
			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( tid : %d, TcpServer::GetHandleMessageList() : %d )",
					(int)syscall(SYS_gettid),
					(MessageList*) GetTcpServer()->GetHandleMessageList()->Size()
					);
			LogManager::GetLogManager()->Log(LOG_WARNING,
					"MatchServer::StateRunnable( tid : %d, TcpServer::GetSendImmediatelyMessageList() : %d )",
					(int)syscall(SYS_gettid),
					(MessageList*) GetTcpServer()->GetSendImmediatelyMessageList()->Size()
					);
//			LogManager::GetLogManager()->Log(LOG_WARNING,
//					"MatchServer::StateRunnable( tid : %d, TcpServer::GetWatcherList() : %d )",
//					(int)syscall(SYS_gettid),
//					(WatcherList*) GetTcpServer()->GetWatcherList()->Size()
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

			iStateTime = miStateTime;
		}
		sleep(1);
	}
}
