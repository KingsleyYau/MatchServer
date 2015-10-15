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
		int iCount = 0;
		while( mContainer->IsRunning() ) {
			if ( iCount < 1 ) {
				iCount++;
			} else {
				iCount = 0;
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetIdleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetIdleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetHandleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetHandleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetSendImmediatelyMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(MessageList*) mContainer->GetTcpServer()->GetSendImmediatelyMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, TcpServer::GetWatcherList() : %d )",
						(int)syscall(SYS_gettid),
						(WatcherList*) mContainer->GetTcpServer()->GetWatcherList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( tid : %d, LogManager::GetIdleMessageList() : %d )",
						(int)syscall(SYS_gettid),
						(WatcherList*) LogManager::GetLogManager()->GetIdleMessageList()->Size()
						);
				LogManager::GetLogManager()->Log(LOG_WARNING,
						"MatchServer::StateRunnable( "
						"tid : %d, "
						"GetTotal() : %u, "
						"GetHit() : %u, "
						"GetIgn() : %u "
						")",
						(int)syscall(SYS_gettid),
						mContainer->GetTotal(),
						mContainer->GetHit(),
						mContainer->GetIgn()
						);
			}
			sleep(1);
		}
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

		ConfFile* conf = new ConfFile();
		conf->InitConfFile(mConfigFile.c_str(), "");
		if ( !conf->LoadConfFile() ) {
			printf("# Match Server can not load config file exit \n");
		}

		short iPort = atoi(conf->GetPrivate("BASE", "PORT", "9876").c_str());
		int iMaxClient = atoi(conf->GetPrivate("BASE", "MAXCLIENT", "100000").c_str());
		int iMaxMemoryCopy = atoi(conf->GetPrivate("BASE", "MAXMEMORYCOPY", "1").c_str());
		int iMaxHandleThread = atoi(conf->GetPrivate("BASE", "MAXHANDLETHREAD", "2").c_str());
		int iMaxQueryPerThread = atoi(conf->GetPrivate("BASE", "MAXQUERYPERCOPY", "10").c_str());

		string host = conf->GetPrivate("DB", "DBHOST", "localhost");
		short dbPort = atoi(conf->GetPrivate("DB", "DBPORT", "3306").c_str());
		string user = conf->GetPrivate("DB", "DBUSER", "root");
		string passwd = conf->GetPrivate("DB", "DBPASS", "123456");
		string dbName = conf->GetPrivate("DB", "DBNAME", "qpidnetwork");
		int iMaxDatabaseThread = atoi(conf->GetPrivate("DB", "MAXDATABASETHREAD", "4").c_str());
		int iSyncDataTime = atoi(conf->GetPrivate("DB", "SYNCHRONIZETIME", "30").c_str());
		mDBManager.SetSyncDataTime(iSyncDataTime * 60);

		int iLogLevel = atoi(conf->GetPrivate("LOG", "LOGLEVEL", "5").c_str());
		string dir = conf->GetPrivate("LOG", "LOGDIR", "log");
		int iDebugMode = atoi(conf->GetPrivate("LOG", "DEBUGMODE", "0").c_str());
		if( iDebugMode == 1 ) {
			LogManager::LogSetFlushBuffer(0);
		} else {
			LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
		}

		delete conf;
		Run(iPort, iMaxClient, iMaxMemoryCopy, iMaxHandleThread, iMaxQueryPerThread, host, dbPort, dbName, user, passwd, iMaxDatabaseThread, iLogLevel, dir);
	} else {
		printf("# No config file can be use exit \n");
	}
}

void MatchServer::Run(
		short iPort,
		int iMaxClient,
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
		) {

	/* log system */
	LogManager::GetLogManager()->Start(1000, iLogLevel, logDir);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::Run( "
			"iPort : %d, "
			"iMaxClient : %d, "
			"iMaxMemoryCopy : %d, "
			"iMaxHandleThread : %d, "
			"iMaxQueryPerThread : %d, "
			"host : %s, "
			"dbPort : %d, "
			"dbName : %s, "
			"user : %s, "
			"passwd : %s, "
			"iMaxDatabaseThread : %d, "
			"iLogLevel : %d, "
			"logDir : %s "
			")",
			iPort,
			iMaxClient,
			iMaxMemoryCopy,
			iMaxHandleThread,
			iMaxQueryPerThread,
			host.c_str(),
			dbPort,
			dbName.c_str(),
			user.c_str(),
			passwd.c_str(),
			iMaxDatabaseThread,
			iLogLevel,
			logDir.c_str()
			);
	bool bFlag = false;

	mTotal = 0;
	mIgn = 0;
	mHit = 0;

	/* db manager */
	bFlag = mDBManager.Init(iMaxMemoryCopy, false);
	bFlag = bFlag && mDBManager.InitSyncDataBase(iMaxDatabaseThread, host.c_str(), dbPort, dbName.c_str(), user.c_str(), passwd.c_str());

	if( !bFlag ) {
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "MatchServer::Run( Database init error )");
		return;
	}

	/* request manager */
	mRequestManager.Init(&mDBManager);
	mRequestManager.SetRequestManagerCallback(this);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( RequestManager Init ok )");

	/* inside server */
	mClientTcpInsideServer.SetTcpServerObserver(this);
	mClientTcpInsideServer.SetHandleSize(1000);
	mClientTcpInsideServer.Start(10, iPort + 1, 2);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( Inside TcpServer Init ok )");

	/* match server */
	mClientTcpServer.SetTcpServerObserver(this);
	/**
	 * 预估相应时间,内存数目*超时间隔*每秒处理的任务
	 */
	mClientTcpServer.SetHandleSize(iMaxMemoryCopy * 5 * iMaxQueryPerThread);
	mClientTcpServer.Start(iMaxClient, iPort, iMaxHandleThread);
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( TcpServer Init ok )");

	mIsRunning = true;

	mpStateThread = new KThread(mpStateRunnable);
	if( mpStateThread->start() != -1 ) {
//		printf("# Create state thread ok \n");
	}

	printf("# MatchServer OK. \n");

	/* call server */
	while( true ) {
		/* do nothing here */
		sleep(3);
	}
}

void MatchServer::Reload() {
	if( mConfigFile.length() > 0 ) {
		ConfFile* conf = new ConfFile();
		conf->InitConfFile(mConfigFile.c_str(), "");
		if ( !conf->LoadConfFile() ) {
			printf("# Match Server can not load config file exit \n");
		}

		int iSyncDataTime = atoi(conf->GetPrivate("DB", "SYNCHRONIZETIME", "30").c_str());
		mDBManager.SetSyncDataTime(iSyncDataTime * 60);

		delete conf;
	}
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
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnRecvMessage( "
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
			mTotal++;
			ret = mRequestManager.HandleRecvMessage(m, sm);
			if( 0 != ret ) {
				if( ret == -1 ) {
					mIgn++;
				} else {
					mHit++;
				}
			}
		} else if( &mClientTcpInsideServer == ts ){
			ret = mRequestManager.HandleInsideRecvMessage(m, sm);
		}

		// Process finish, send respond
		ts->SendMessageByQueue(sm);

	} else {
		LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnRecvMessage( "
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
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnRecvMessage( "
			"tid : %d, "
			"m->fd : [%d], "
			"end "
			")",
			(int)syscall(SYS_gettid),
			m->fd
			);
}

void MatchServer::OnSendMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnSendMessage( tid : %d, m->fd : [%d], start )", (int)syscall(SYS_gettid), m->fd);
	// 发送成功，断开连接
	ts->Disconnect(m->fd);
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnSendMessage( tid : %d, m->fd : [%d], end )", (int)syscall(SYS_gettid), m->fd);
}

void MatchServer::OnTimeoutMessage(TcpServer *ts, Message *m) {
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnTimeoutMessage( tid : %d, m->fd : [%d], start )", (int)syscall(SYS_gettid), m->fd);
	Message *sm = ts->GetIdleMessageList()->PopFront();
	if( sm != NULL ) {
		sm->fd = m->fd;
		sm->wr = m->wr;
		mTotal++;
		int ret = mRequestManager.HandleTimeoutMessage(m, sm);
		// Process finish, send respond
		ts->SendMessageByQueue(sm);
		mIgn++;

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
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnTimeoutMessage( tid : %d, m->fd : [%d], end )", (int)syscall(SYS_gettid), m->fd);
}

/**
 * OnDisconnect
 */
void MatchServer::OnDisconnect(TcpServer *ts, int fd) {
	LogManager::GetLogManager()->Log(LOG_MSG, "MatchServer::OnDisconnect( tid: %d, fd : [%d] )", (int)syscall(SYS_gettid), fd);
}

void MatchServer::OnReload(RequestManager* pRequestManager) {
	Reload();
}

unsigned int MatchServer::GetTotal() {
	return mTotal;
}

unsigned int MatchServer::GetHit() {
	return mHit;
}

unsigned int MatchServer::GetIgn() {
	return mIgn;
}
