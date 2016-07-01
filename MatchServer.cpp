/*
 * MatchServer.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "MatchServer.h"
#include "DataHttpParser.h"

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
	// 基本参数
	miPort = 0;
	miMaxClient = 0;
	miMaxHandleThread = 0;
	miMaxMemoryCopy = 0;
	miMaxQueryPerThread = 0;
	miTimeout = 0;

	// 数据库参数
	miSyncOnlineTime = 0;
	miSyncTime = 0;
	miOnlineDbCount = 0;

	// 日志参数
	miStateTime = 0;
	miDebugMode = 0;
	miLogLevel = 0;

	// 统计参数
	mTotal = 0;
	mHit = 0;
	mResponedTime = 0;
	mHandleTime = 0;

	// 其他
	mIsRunning = false;

	// 运行参数
	mpStateRunnable = new StateRunnable(this);
	mpStateThread = NULL;
}

MatchServer::~MatchServer() {
	// TODO Auto-generated destructor stub
	if( mpStateRunnable ) {
		delete mpStateRunnable;
	}
}

bool MatchServer::Run(const string& config) {
	if( config.length() > 0 ) {
		mConfigFile = config;

		// Reload config
		if( Reload() ) {
			if( miDebugMode == 1 ) {
				LogManager::LogSetFlushBuffer(0);
			} else {
				LogManager::LogSetFlushBuffer(5 * BUFFER_SIZE_1K * BUFFER_SIZE_1K);
			}

			return Run();
		} else {
			printf("# Match Server can not load config file exit. \n");
		}
	} else {
		printf("# No config file can be use exit. \n");
	}

	return false;
}

bool MatchServer::Run() {
	/* log system */
	LogManager::GetLogManager()->Start(1000, miLogLevel, mLogDir);

	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"MatchServer::Run( "
			"############## Match Server ############## "
			")"
			);
	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"MatchServer::Run( "
			"Version : %s "
			")",
			VERSION_STRING
			);
	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"MatchServer::Run( "
			"Build date : %s %s "
			")",
			__DATE__,
			__TIME__
			);

	LogManager::GetLogManager()->Log(
			LOG_ERR_SYS,
			"MatchServer::Run( "
			"[启动, 初始化基本配置], "
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
			LOG_ERR_SYS,
			"MatchServer::Run( "
			"[启动, 初始化数据库配置], "
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
				LOG_ERR_SYS,
				"MatchServer::Run( "
				"[启动, 初始化数据库配置], "
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

	/**
	 * 客户端解析包缓存buffer
	 */
	for(int i = 0; i < miMaxClient; i++) {
		/* create idle buffers */
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}

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
		LogManager::GetLogManager()->Log(LOG_ERR_SYS, "MatchServer::Run( [启动, 初始化数据库失败] )");
		return false;
	}

	/* 外部服务(HTTP) */
	mTcpServerPublic.SetTcpServerObserver(this);
	mTcpServerPublic.SetHandleSize(1000);
	mTcpServerPublic.Start(1000, miPort + 1, 2);

	/**
	 * 内部服务(HTTP), 预估相应时间,内存数目*超时间隔*每秒处理的任务
	 */
	mTcpServer.SetTcpServerObserver(this);
	mTcpServer.SetHandleSize(miMaxMemoryCopy * miTimeout * miMaxQueryPerThread);
	mTcpServer.Start(miMaxClient, miPort, miMaxHandleThread);

	/**
	 * Match Server
	 */
	mIsRunning = true;

	mpStateThread = new KThread(mpStateRunnable);
	if( mpStateThread->start() != 0 ) {
	}

	printf("# MatchServer start OK. \n");
	LogManager::GetLogManager()->Log(LOG_WARNING, "MatchServer::Run( [启动, 成功] )");

	return true;
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

			mTcpServer.SetHandleSize(miMaxMemoryCopy * miTimeout * miMaxQueryPerThread);

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

bool MatchServer::OnAccept(TcpServer *ts, int fd, char* ip) {
	Client *client = new Client();
	client->SetClientCallback(this);
	client->SetMessageList(&mIdleMessageList);
	client->fd = fd;
	client->ip = ip;
	client->isOnline = true;

	// 记录在线客户端
	mClientMap.Lock();
	mClientMap.Insert(fd, client);
	mClientMap.Unlock();

	if( ts == &mTcpServer ) {
		// 统计请求
		mCountMutex.lock();
		mTotal++;
		mCountMutex.unlock();

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnAccept( "
				"tid : %d, "
				"[内部服务(HTTP), 建立连接], "
				"fd : [%d], "
				"client : %p "
				")",
				(int)syscall(SYS_gettid),
				fd,
				client
				);

	} else if( &mTcpServerPublic == ts ) {
		// 外部服务(HTTP)
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnAccept( "
				"tid : %d, "
				"[外部服务(HTTP), 建立连接], "
				"fd : [%d], "
				"client : %p "
				")",
				(int)syscall(SYS_gettid),
				fd,
				client
				);
	}

	return true;
}

void MatchServer::OnDisconnect(TcpServer *ts, int fd) {
	mClientMap.Lock();
	ClientMap::iterator itr = mClientMap.Find(fd);
	if( itr != mClientMap.End() ) {
		Client* client = itr->second;
		if( client != NULL ) {
			client->isOnline = false;

			if( ts == &mTcpServer ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::OnDisconnect( "
						"tid : %d, "
						"[内部服务(HTTP), 断开连接, 找到对应连接], "
						"fd : [%d], "
						"client : %p "
						")",
						(int)syscall(SYS_gettid),
						fd,
						client
						);

			} else if( &mTcpServerPublic == ts ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::OnDisconnect( "
						"tid : %d, "
						"[外部服务(HTTP), 断开连接, 找到对应连接], "
						"fd : [%d], "
						"client : %p "
						")",
						(int)syscall(SYS_gettid),
						fd,
						client
						);

			}

		} else {
			if( ts == &mTcpServer ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"MatchServer::OnDisconnect( "
						"tid : %d, "
						"[内部服务(HTTP), 断开连接, 找不到对应连接, client ==  NULL], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);

			} else if( &mTcpServerPublic == ts ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"MatchServer::OnDisconnect( "
						"tid : %d, "
						"[外部服务(HTTP), 断开连接, 找不到对应连接, client ==  NULL], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
			}
		}
	} else {
		if( ts == &mTcpServer ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"MatchServer::OnDisconnect( "
					"tid : %d, "
					"[内部服务(HTTP), 断开连接, 找不到对应连接], "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

		} else if( &mTcpServerPublic == ts ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"MatchServer::OnDisconnect( "
					"tid : %d, "
					"[外部服务(HTTP), 断开连接, 找不到对应连接], "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
		}

	}
	mClientMap.Unlock();
}

void MatchServer::OnClose(TcpServer *ts, int fd) {
	// 释放资源下线
	mClientMap.Lock();
	ClientMap::iterator itr = mClientMap.Find(fd);
	if( itr != mClientMap.End() ) {
		Client* client = itr->second;

		if( client != NULL ) {
			if( ts == &mTcpServer ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::OnClose( "
						"tid : %d, "
						"[内部服务(HTTP), 关闭socket, 找到对应连接], "
						"fd : [%d], "
						"client : %p "
						")",
						(int)syscall(SYS_gettid),
						fd,
						client
						);

			} else if( &mTcpServerPublic == ts ) {
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::OnClose( "
						"tid : %d, "
						"[外部服务(HTTP), 关闭socket, 找到对应连接], "
						"fd : [%d], "
						"client : %p "
						")",
						(int)syscall(SYS_gettid),
						fd,
						client
						);

			}
			delete client;

		} else {
			if( ts == &mTcpServer ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"MatchServer::OnClose( "
						"tid : %d, "
						"[内部服务(HTTP), 关闭socket, 找不到对应连接, client ==  NULL], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);

			} else if( &mTcpServerPublic == ts ) {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"MatchServer::OnClose( "
						"tid : %d, "
						"[外部服务(HTTP), 关闭socket, 找不到对应连接, client ==  NULL], "
						"fd : [%d] "
						")",
						(int)syscall(SYS_gettid),
						fd
						);
			}

		}

		mClientMap.Erase(itr);

	} else {
		if( ts == &mTcpServer ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"MatchServer::OnClose( "
					"tid : %d, "
					"[内部服务(HTTP), 关闭socket, 找不到对应连接], "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);

		} else if( &mTcpServerPublic == ts ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"MatchServer::OnClose( "
					"tid : %d, "
					"[外部服务(HTTP), 关闭socket, 找不到对应连接], "
					"fd : [%d] "
					")",
					(int)syscall(SYS_gettid),
					fd
					);
		}

	}
	mClientMap.Unlock();

}

void MatchServer::OnRecvMessage(TcpServer *ts, Message *m) {
	// 接收完成
	if( &mTcpServer == ts ) {
		// 内部服务(HTTP), 接收完成处理
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnRecvMessage( "
				"tid : %d, "
				"[内部服务(HTTP), 收到数据], "
				"fd : [%d], "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				m->len
				);

		TcpServerRecvMessageHandle(ts, m);

	} else if( &mTcpServerPublic == ts ) {
		// 外部服务(HTTP), 接收完成处理
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnRecvMessage( "
				"tid : %d, "
				"[外部服务(HTTP), 收到数据], "
				"fd : [%d], "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				m->len
				);

		TcpServerPublicRecvMessageHandle(ts, m);
	}
}

void MatchServer::OnSendMessage(TcpServer *ts, Message *m) {
	if( &mTcpServer == ts ) {
		// 内部服务(HTTP)
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnSendMessage( "
				"tid : %d, "
				"[内部服务(HTTP), 发送数据], "
				"fd : [%d], "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				m->len
				);

	} else if( &mTcpServerPublic == ts ) {
		// 外部服务(HTTP)
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnSendMessage( "
				"tid : %d, "
				"[外部服务(HTTP), 发送数据], "
				"fd : [%d], "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				m->len
				);
	}

	// 发送完成
	mClientMap.Lock();
	ClientMap::iterator itr = mClientMap.Find(m->fd);
	if( itr != mClientMap.End() ) {
		Client* client = itr->second;
		if( client != NULL ) {
			client->AddSentPacket();
			if( client->IsAllPacketSent() ) {
				// 发送完成处理
				ts->Disconnect(m->fd);
			}
		}
	}
	mClientMap.Unlock();
}

void MatchServer::OnTimeoutMessage(TcpServer *ts, Message *m) {
	// 接收超时
	if( &mTcpServer == ts ) {
		// 内部服务(HTTP)
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"MatchServer::OnTimeoutMessage( "
				"tid : %d, "
				"[内部服务(HTTP), 收到超时数据], "
				"fd : [%d], "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				m->len
				);

		TcpServerTimeoutMessageHandle(ts, m);

	} else {
		// 外部服务(HTTP)
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::OnTimeoutMessage( "
				"tid : %d, "
				"[外部服务(HTTP), 收到超时数据], "
				"fd : [%d], "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				m->len
				);

		TcpServerPublicTimeoutMessageHandle(ts, m);

	}

}

void MatchServer::TcpServerRecvMessageHandle(TcpServer *ts, Message *m) {
	int ret = 0;

	if( m->buffer != NULL && m->len > 0 ) {
		Client *client = NULL;

		/**
		 * 因为还有数据包处理队列中, TcpServer不会回调OnClose, 所以不怕client被释放
		 * 放开锁就可以使多个client并发解数据包, 单个client解包只能同步解包, 在client内部加锁
		 */
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(m->fd);
		if( itr != mClientMap.End() ) {
			client = itr->second;
			mClientMap.Unlock();

		} else {
			mClientMap.Unlock();

		}

		if( client != NULL ) {
			if ( DiffGetTickCount(m->starttime, GetTickCount()) < miTimeout * 1000 ) {
				// 解析数据包
				LogManager::GetLogManager()->Log(
						LOG_MSG,
						"MatchServer::TcpServerRecvMessageHandle( "
						"tid : %d, "
						"[内部服务(HTTP), 处理客户端请求], "
						"fd : [%d], "
						"buffer : [\n%s\n]"
						")",
						(int)syscall(SYS_gettid),
						client->fd,
						m->buffer
						);

				ret = client->ParseData(m);

			} else {
				// 超时不解析
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"MatchServer::TcpServerRecvMessageHandle( "
						"tid : %d, "
						"[内部服务(HTTP), 处理客户端请求, 超时不处理, 发送返回], "
						"fd : [%d], "
						"buffer : [\n%s\n]"
						")",
						(int)syscall(SYS_gettid),
						client->fd,
						m->buffer
						);
				client->starttime = m->starttime;

				Json::Value root;
				Json::Value womanListNode;
				root["womaninfo"] = womanListNode;

				// 发送返回
				SendRespond2Client(&mTcpServer, client, root, false);
			}
		}
	}

	if( ret == -1 ) {
		ts->Disconnect(m->fd);
	}
}

void MatchServer::TcpServerTimeoutMessageHandle(TcpServer *ts, Message *m) {
	Client *client = NULL;

	/**
	 * 因为还有数据包处理队列中, TcpServer不会回调OnClose, 所以不怕client被释放
	 * 放开锁就可以使多个client并发解数据包, 单个client解包只能同步解包, 在client内部加锁
	 */
	mClientMap.Lock();
	ClientMap::iterator itr = mClientMap.Find(m->fd);
	if( itr != mClientMap.End() ) {
		client = itr->second;
		mClientMap.Unlock();

	} else {
		mClientMap.Unlock();

	}

	if( client != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::TcpServerRecvMessageHandle( "
				"tid : %d, "
				"[内部服务(HTTP), 处理客户端超时请求, 发送返回], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				client->fd
				);
		client->starttime = m->starttime;

		Json::Value root;
		Json::Value womanListNode;
		root["womaninfo"] = womanListNode;

		// 发送返回
		SendRespond2Client(&mTcpServer, client, root, false);
	}
}

void MatchServer::TcpServerPublicRecvMessageHandle(TcpServer *ts, Message *m) {
	int ret = 0;

	if( m->buffer != NULL && m->len > 0 ) {
		Client *client = NULL;

		/**
		 * 因为还有数据包处理队列中, TcpServer不会回调OnClose, 所以不怕client被释放
		 * 放开锁就可以使多个client并发解数据包, 单个client解包只能同步解包, 在client内部加锁
		 */
		mClientMap.Lock();
		ClientMap::iterator itr = mClientMap.Find(m->fd);
		if( itr != mClientMap.End() ) {
			client = itr->second;
			mClientMap.Unlock();

		} else {
			mClientMap.Unlock();

		}

		if( client != NULL ) {
			// 解析数据包
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"MatchServer::TcpServerPublicRecvMessageHandle( "
					"tid : %d, "
					"[外部服务(HTTP), 处理客户端请求], "
					"fd : [%d], "
					"buffer : [\n%s\n]"
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					m->buffer
					);

			ret = client->ParseData(m);
		}
	}

	if( ret == -1 ) {
		ts->Disconnect(m->fd);
	}
}

void MatchServer::TcpServerPublicTimeoutMessageHandle(TcpServer *ts, Message *m) {
}

/***************************** 内部服务(HTTP)接口 **************************************/
bool MatchServer::SendRespond2Client(
		TcpServer *ts,
		Client* client,
		Json::Value& root,
		bool success,
		bool found
		) {
	bool bFlag = false;

	if( &mTcpServer == ts ) {
		// 统计请求
		long long respondTime = GetTickCount() - client->starttime;
		AddResopnd(success, respondTime);

		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::SendRespond2Client( "
				"tid : %d, "
				"[内部服务(HTTP), 返回请求到客户端], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				client->fd
				);

	} else if( &mTcpServerPublic == ts ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::SendRespond2Client( "
				"tid : %d, "
				"[外部服务(HTTP), 返回请求到客户端], "
				"fd : [%d] "
				")",
				(int)syscall(SYS_gettid),
				client->fd
				);
	}

	// 发送头部
	Message* m = ts->GetIdleMessageList()->PopFront();
	if( m != NULL ) {
		client->AddSendingPacket();

		m->fd = client->fd;
		snprintf(
				m->buffer,
				MAX_BUFFER_LEN - 1,
				"HTTP/1.1 %d %s\r\n"
				"Connection:Keep-Alive\r\n"
				"Content-Type:text/html; charset=utf-8\r\n"
				"\r\n",
				found?200:404,
				found?"OK":"Not Found"
				);
		m->len = strlen(m->buffer);

		if( &mTcpServer == ts ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"MatchServer::SendRespond2Client( "
					"tid : %d, "
					"[内部服务(HTTP), 返回请求头部到客户端], "
					"fd : [%d], "
					"buffer : [\n%s\n]"
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					m->buffer
					);

		} else if( &mTcpServerPublic == ts ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"MatchServer::SendRespond2Client( "
					"tid : %d, "
					"[外部服务(HTTP), 返回请求头部到客户端], "
					"fd : [%d], "
					"buffer : [\n%s\n]"
					")",
					(int)syscall(SYS_gettid),
					client->fd,
					m->buffer
					);
		}

		ts->SendMessageByQueue(m);
	}

//	if( respond != NULL ) {
		// 发送内容
		bool more = false;
		while( true ) {
			Message* m = ts->GetIdleMessageList()->PopFront();
			if( m != NULL ) {
				m->fd = client->fd;
				client->AddSendingPacket();

				string param;
				Json::FastWriter writer;
				param = writer.write(root);
				m->len = param.length();
				sprintf(m->buffer, "%s", param.c_str());

				if( &mTcpServer == ts ) {
					LogManager::GetLogManager()->Log(
							LOG_WARNING,
							"MatchServer::SendRespond2Client( "
							"tid : %d, "
							"[内部服务(HTTP), 返回请求内容到客户端], "
							"fd : [%d], "
							"buffer : [\n%s\n]"
							")",
							(int)syscall(SYS_gettid),
							client->fd,
							m->buffer
							);

				} else if( &mTcpServerPublic == ts ) {
					LogManager::GetLogManager()->Log(
							LOG_WARNING,
							"MatchServer::SendRespond2Client( "
							"tid : %d, "
							"[外部服务(HTTP), 返回请求内容到客户端], "
							"fd : [%d], "
							"buffer : [\n%s\n]"
							")",
							(int)syscall(SYS_gettid),
							client->fd,
							m->buffer
							);
				}

				ts->SendMessageByQueue(m);
				if( !more ) {
					// 全部发送完成
					bFlag = true;
					break;
				}

			} else {
				break;
			}
		}
//	}

	if( &mTcpServer == ts ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::SendRespond2Client( "
				"tid : %d, "
				"[内部服务(HTTP), 返回请求到客户端, 完成], "
				"fd : [%d], "
				"bFlag : %s "
				")",
				(int)syscall(SYS_gettid),
				client->fd,
				bFlag?"true":"false"
				);

	} else if( &mTcpServerPublic == ts ) {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"MatchServer::SendRespond2Client( "
				"tid : %d, "
				"[外部服务(HTTP), 返回请求到客户端], "
				"fd : [%d], "
				"bFlag : %s "
				")",
				(int)syscall(SYS_gettid),
				client->fd,
				bFlag?"true":"false"
				);
	}

	return bFlag;
}
/* callback by DBManager */
void MatchServer::OnSyncFinish(DBManager* pDBManager) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnSyncFinish( "
			"tid : %d "
			"[同步数据库记录完成] "
			")",
			(int)syscall(SYS_gettid)
			);

}

void MatchServer::AddResopnd(bool hit, long long respondTime) {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"MatchServer::AddResopnd( "
			"tid : %d, "
			"[内部服务(HTTP), 统计返回时间], "
			"hit : %s, "
			"respondTime : %lld ms "
			")",
			(int)syscall(SYS_gettid),
			hit?"true":"false",
			respondTime
			);

	// 统计请求
	mCountMutex.lock();
	mResponedTime += respondTime;
	if( hit ) {
		mHit++;
	}
	mCountMutex.unlock();
}

void MatchServer::AddHandleTime(long long time) {
	// 统计处理时间
	mCountMutex.lock();
	mHandleTime += time;
	mCountMutex.unlock();
}

void MatchServer::StateRunnableHandle() {
	unsigned int iCount = 0;
	// 请求总数
	unsigned int iTotal = 0;
	double iSecondTotal = 0;
	// 处理总数
	unsigned int iHit = 0;
	double iSecondHit = 0;
	// 响应时间
	double iResponedTime = 0;
	// 处理时间
	double iHandleTime = 0;
	// 统计时间
	unsigned int iStateTime = miStateTime;

	while( IsRunning() ) {
		if ( iCount < iStateTime ) {
			iCount++;
		} else {
			iCount = 0;
			iSecondTotal = 0;
			iSecondHit = 0;
			iResponedTime = 0;

			mCountMutex.lock();
			iTotal = mTotal;
			iHit = mHit;

			if( iStateTime != 0 ) {
				iSecondTotal = 1.0 * iTotal / iStateTime;
				iSecondHit = 1.0 * iHit / iStateTime;
			}
			if( iTotal != 0 ) {
				iResponedTime = 1.0 * mResponedTime / iTotal;
			}
			if( iHit != 0 ) {
				iHandleTime = 1.0 * mHandleTime / iHit;
			}

			mHit = 0;
			mTotal = 0;
			mResponedTime = 0;
			mHandleTime = 0;
			mCountMutex.unlock();

			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"MatchServer::StateRunnableHandle( "
					"tid : %d, "
					"过去%u秒共收到%u个请求, "
					"平均收到%.1lf个/秒, "
					"成功处理%u个请求, "
					"平均处理%.1lf个/秒, "
					"平均处理时间%.1lf毫秒/个, "
					"平均响应时间%.1lf毫秒/个"
					")",
					(int)syscall(SYS_gettid),
					iStateTime,
					iTotal,
					iSecondTotal,
					iHit,
					iSecondHit,
					iHandleTime,
					iResponedTime
					);

			// 统计问题数目
			mDBManager.CountStatus();

			iStateTime = miStateTime;
		}
		sleep(1);
	}
}

/***************************** 内部服务(HTTP) 回调处理 **************************************/
void MatchServer::OnClientQuerySameAnswerLadyList(
		Client* client,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		long long iRegTime
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientQuerySameAnswerLadyList( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取跟男士有任意共同答案的问题的女士Id列表], "
			"fd : [%d], "
			"pManId : %s, "
			"pSiteId : %s, "
			"pLimit : %s, "
			"iRegTime : %lld, "
			"client->index : %d "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			pManId,
			pSiteId,
			pLimit,
			iRegTime,
			client->index
			);

	Json::Value root;
	Json::Value womanListNode;
	if( (pManId != NULL) && strlen(pManId) > 0 && (pSiteId != NULL) && strlen(pSiteId) > 0 ) {
		if( QuerySameAnswerLadyList(womanListNode, pManId, pSiteId, pLimit, iRegTime, client->index) ) {

		}
	}
	root["womaninfo"] = womanListNode;

	// 发送返回
	SendRespond2Client(&mTcpServer, client, root);
}

void MatchServer::OnClientQueryTheSameQuestionLadyList(
		Client* client,
		const char* pQId,
		const char* pSiteId,
		const char* pLimit
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientQueryTheSameQuestionLadyList( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取有指定共同问题的女士Id列表], "
			"fd : [%d], "
			"pQId : %s, "
			"pSiteId : %s, "
			"pLimit : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			pQId,
			pSiteId,
			pLimit
			);

	Json::Value root;
	Json::Value womanListNode;
	if( (pQId != NULL) && strlen(pQId) > 0 && (pSiteId != NULL) && strlen(pSiteId) > 0 ) {
		if( QueryTheSameQuestionLadyList(womanListNode, pQId, pSiteId, pLimit, client->index) ) {
		}
	}
	root["womaninfo"] = womanListNode;

	// 发送返回
	SendRespond2Client(&mTcpServer, client, root);
}

void MatchServer::OnClientQueryAnySameQuestionLadyList(
		Client* client,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientQueryAnySameQuestionLadyList( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取有任意共同问题的女士Id列表], "
			"fd : [%d], "
			"pManId : %s, "
			"pSiteId : %s, "
			"pLimit : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			pManId,
			pSiteId,
			pLimit
			);

	Json::Value root;
	Json::Value womanListNode;
	if( (pManId != NULL) && strlen(pManId) > 0 && (pSiteId != NULL) && strlen(pSiteId) > 0 ) {
		if( QueryAnySameQuestionLadyList(womanListNode, pManId, pSiteId, pLimit, client->index) ) {
		}
	}
	root["womaninfo"] = womanListNode;

	// 发送返回
	SendRespond2Client(&mTcpServer, client, root);
}

void MatchServer::OnClientQuerySameQuestionOnlineLadyList(
		Client* client,
		const char* pQIds,
		const char* pSiteId,
		const char* pLimit
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientQuerySameQuestionOnlineLadyList( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取回答过注册问题的在线女士Id列表], "
			"fd : [%d], "
			"pQIds : %s, "
			"pSiteId : %s, "
			"pLimit : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			pQIds,
			pSiteId,
			pLimit
			);

	Json::Value root;
	Json::Value womanListNode;
	if( (pQIds != NULL) && strlen(pQIds) > 0 && (pSiteId != NULL) && strlen(pSiteId) > 0 ) {
		if( QuerySameQuestionOnlineLadyList(womanListNode, pQIds, pSiteId, pLimit, client->index) ) {
		}
	}
	root["womaninfo"] = womanListNode;

	// 发送返回
	SendRespond2Client(&mTcpServer, client, root);
}

void MatchServer::OnClientQuerySameQuestionAnswerLadyList(
		Client* client,
		const char* pQIds,
		const char* pAIds,
		const char* pSiteId,
		const char* pLimit
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientQuerySameQuestionAnswerLadyList( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取指定问题有共同答案的女士Id列表], "
			"fd : [%d], "
			"pQIds : %s, "
			"pAIds : %s, "
			"pSiteId : %s, "
			"pLimit : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			pQIds,
			pAIds,
			pSiteId,
			pLimit
			);

	Json::Value root;
	Json::Value womanListNode;
	if(
		(pQIds != NULL) && strlen(pQIds) > 0 &&
		(pAIds != NULL) && strlen(pAIds) > 0 &&
		(pSiteId != NULL) && strlen(pSiteId) > 0
		) {
		if( QuerySameQuestionAnswerLadyList(womanListNode, pQIds, pAIds, pSiteId, pLimit, client->index) ) {
		}
	}
	root["womaninfo"] = womanListNode;

	// 发送返回
	SendRespond2Client(&mTcpServer, client, root);
}

void MatchServer::OnClientQuerySameQuestionLadyList(
		Client* client,
		const char* pQIds,
		const char* pSiteId,
		const char* pLimit
		) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientQuerySameQuestionLadyList( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:获取回答过注册问题的女士Id列表], "
			"fd : [%d], "
			"pQIds : %s, "
			"pSiteId : %s, "
			"pLimit : %s "
			")",
			(int)syscall(SYS_gettid),
			client->fd,
			pQIds,
			pSiteId,
			pLimit
			);

	Json::Value root;
	Json::Value womanListNode;
	if( (pQIds != NULL) && strlen(pQIds) > 0 && (pSiteId != NULL) && strlen(pSiteId) > 0 ) {
		if( QuerySameQuestionLadyList(womanListNode, pQIds, pSiteId, pLimit, client->index) ) {
		}
	}
	root["womaninfo"] = womanListNode;

	// 发送返回
	SendRespond2Client(&mTcpServer, client, root);
}

void MatchServer::OnClientSync(Client* client) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientSync( "
			"tid : %d, "
			"[外部服务(HTTP), 收到命令:同步数据库], "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			client->fd
			);
	// 接口已经废弃, 直接返回
	Json::Value root = "{ret:1}";
	// 发送返回
	SendRespond2Client(&mTcpServer, client, root, false, false);
}

void MatchServer::OnClientUndefinedCommand(Client* client) {
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::OnClientUndefinedCommand( "
			"tid : %d, "
			"[内部服务(HTTP), 收到命令:未知命令], "
			"fd : [%d] "
			")",
			(int)syscall(SYS_gettid),
			client->fd
			);
	Json::Value root = "";
	// 发送返回
	SendRespond2Client(&mTcpServer, client, root, false, false);
}

/***************************** 内部服务(HTTP) 回调处理 end **************************************/
/**
 * 1.获取跟男士有任意共同答案的问题的女士Id列表
 */
bool MatchServer::QuerySameAnswerLadyList(
		Json::Value& womanListNode,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		long long iRegTime,
		int iQueryIndex
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	time_t t;
	long long unixTime = time(&t);

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"[获取跟男士有任意共同答案的问题的女士Id列表], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iRegTime : %lld, "
			"unixTime : %lld, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			pManId,
			pSiteId,
			pLimit,
			iRegTime,
			unixTime,
			iQueryIndex
			);

	// 执行查询
	char sql[1024] = {'\0'};

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	int iManCount = 0;
	long long iRegTimeout = unixTime - iRegTime;
	sprintf(sql, "SELECT count(*) aid FROM mq_man_answer WHERE manid = '%s';", pManId);
	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	if( bResult && result && iRow > 0 ) {
		iManCount = atoi(result[1 * iColumn]);
	}
	mDBManager.FinishQuery(result);

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"[获取跟男士有任意共同答案的问题的女士Id列表], "
			"iManCount : %d, "
			"iRegTimeout : %lld "
			")",
			(int)syscall(SYS_gettid),
			iManCount,
			iRegTimeout
			);
	if( (iManCount == 0) && (iRegTimeout < (long long)miSyncTime * 60) ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"MatchServer::QuerySameAnswerLadyList( "
				"tid : %d, "
				"[获取跟男士有任意共同答案的问题的女士Id列表, 没有男士问题并且注册时间没有超时] "
				")",
				(int)syscall(SYS_gettid),
				iManCount,
				iRegTimeout
				);

		// 同步
		mDBManager.SyncForce();
		bSleepAlready = true;
	}

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	sprintf(sql, "SELECT qid, aid FROM mq_man_answer WHERE manid = '%s';", pManId);
	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"[获取跟男士有任意共同答案的问题的女士Id列表], "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
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
									"[获取跟男士有任意共同答案的问题的女士Id列表], "
									"Count iQueryTime : %u m, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
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
									"[获取跟男士有任意共同答案的问题的女士Id列表], "
									"Query iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iRow2 : %d, "
									"iColumn2 : %d, "
									"iLadyCount : %d, "
									"iLadyIndex : %d, "
									"womanidMap.size() : %d "
									")",
									(int)syscall(SYS_gettid),
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
								"[获取跟男士有任意共同答案的问题的女士Id列表], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
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
								"[获取跟男士有任意共同答案的问题的女士Id列表], "
								"Double Check iQueryTime : %u ms, "
								"iQueryIndex : %d, "
								"iSingleQueryTime : %u ms "
								")",
								(int)syscall(SYS_gettid),
								iQueryTime,
								iQueryIndex,
								iSingleQueryTime
								);

			i++;
			i = ((i - 1) % iRow) + 1;

			// 单次请求是否大于30ms
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
	AddHandleTime(iHandleTime);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::QuerySameAnswerLadyList( "
			"tid : %d, "
			"[获取跟男士有任意共同答案的问题的女士Id列表], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iRegTime : %lld, "
			"unixTime : %lld, "
			"iQueryIndex : %d, "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			pManId,
			pSiteId,
			pLimit,
			iRegTime,
			unixTime,
			iQueryIndex,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

/**
 * 2.获取跟男士有指定共同问题的女士Id列表
 */
bool MatchServer::QueryTheSameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pQid,
		const char* pSiteId,
		const char* pLimit,
		int iQueryIndex
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();

	Json::Value womanNode;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"[获取跟男士有指定共同问题的女士Id列表], "
			"qid : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			pQid,
			pSiteId,
			pLimit,
			iQueryIndex
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
			"[获取跟男士有指定共同问题的女士Id列表], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
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
							"[获取跟男士有指定共同问题的女士Id列表], "
							"Count iQueryTime : %u ms, "
							"iQueryIndex : %d, "
							"iNum : %d, "
							"iMax : %d "
							")",
							(int)syscall(SYS_gettid),
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
							"[获取跟男士有指定共同问题的女士Id列表], "
							"Query iQueryTime : %u ms, "
							"iQueryIndex : %d, "
							"iRow2 : %d, "
							"iColumn2 : %d, "
							"iLadyCount : %d, "
							"iLadyIndex : %d "
							")",
							(int)syscall(SYS_gettid),
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
	AddHandleTime(iHandleTime);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"[获取跟男士有指定共同问题的女士Id列表], "
			"qid : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			pQid,
			pSiteId,
			pLimit,
			iQueryIndex,
			iHandleTime
			);

	return bResult;
}

/**
 * 3.获取跟男士有任意共同问题的女士Id列表
 */
bool MatchServer::QueryAnySameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		int iQueryIndex
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"[获取跟男士有任意共同问题的女士Id列表], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			pManId,
			pSiteId,
			pLimit,
			iQueryIndex
			);

	// 执行查询
	char sql[1024] = {'\0'};
	sprintf(sql, "SELECT qid FROM mq_man_answer "
			"WHERE manid = '%s';",
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
			"[获取跟男士有任意共同问题的女士Id列表], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
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
						"[获取跟男士有任意共同问题的女士Id列表], "
						"Count iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iNum : %d, "
						"iMax : %d "
						")",
						(int)syscall(SYS_gettid),
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
						"[获取跟男士有任意共同问题的女士Id列表], "
						"Query iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iRow2 : %d, "
						"iColumn2 : %d, "
						"iLadyCount : %d, "
						"iLadyIndex : %d, "
						"womanidMap.size() : %d "
						")",
						(int)syscall(SYS_gettid),
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
								"[获取跟男士有任意共同问题的女士Id列表], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
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
	AddHandleTime(iHandleTime);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"[获取跟男士有任意共同问题的女士Id列表], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d, "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			pManId,
			pSiteId,
			pLimit,
			iQueryIndex,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

/**
 * 4.获取回答过注册问题的在线女士Id列表
 */
bool MatchServer::QuerySameQuestionOnlineLadyList(
		Json::Value& womanListNode,
		const char* pQids,
		const char* pSiteId,
		const char* pLimit,
		int iQueryIndex
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameQuestionOnlineLadyList( "
			"tid : %d, "
			"[获取回答过注册问题的在线女士Id列表], "
			"qids : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			pQids,
			pSiteId,
			pLimit,
			iQueryIndex
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
									"MatchServer::QuerySameQuestionOnlineLadyList( "
									"tid : %d, "
									"[获取回答过注册问题的在线女士Id列表], "
									"Count iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
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
						"MatchServer::QuerySameQuestionOnlineLadyList( "
						"tid : %d, "
						"[获取回答过注册问题的在线女士Id列表], "
						"Query iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iRow2 : %d, "
						"iColumn2 : %d, "
						"iLadyCount : %d, "
						"iLadyIndex : %d, "
						"womanidMap.size() : %d "
						")",
						(int)syscall(SYS_gettid),
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
								"MatchServer::QuerySameQuestionOnlineLadyList( "
								"tid : %d, "
								"[获取回答过注册问题的在线女士Id列表], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
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
	AddHandleTime(iHandleTime);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::QuerySameQuestionOnlineLadyList( "
			"tid : %d, "
			"[获取回答过注册问题的在线女士Id列表], "
			"qids : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d, "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			pQids,
			pSiteId,
			pLimit,
			iQueryIndex,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

/**
 * 5.获取回答过注册问题有共同答案的女士Id列表
 */
bool MatchServer::QuerySameQuestionAnswerLadyList(
		Json::Value& womanListNode,
		const char* pQids,
		const char* pAids,
		const char* pSiteId,
		const char* pLimit,
		int iQueryIndex
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameQuestionAnswerLadyList( "
			"tid : %d, "
			"[获取回答过注册问题有共同答案的女士Id列表], "
			"qids : %s, "
			"aids : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			pQids,
			pAids,
			pSiteId,
			pLimit,
			iQueryIndex
			);

	// 执行查询
	char sql[1024] = {'\0'};

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bool bResult = false;
	int iCount = 0;

	if( pQids != NULL && pAids != NULL ) {
		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		bool bEnougthLady = false;

		string qpids = pQids;
		qpids += ",";
		string qid;

		string aids = pAids;
		aids += ",";
		string aid;

	    string::size_type posQid;
	    string::size_type posAid;

	    int i = 0;
	    int j = 0;
	    posQid = qpids.find(",", i);
	    posAid = aids.find(",", j);

		while( posQid != string::npos && posAid != string::npos && iCount < 7 ) {
			iSingleQueryTime = GetTickCount();

			qid = qpids.substr(i, posQid - i);
			aid = aids.substr(j, posAid - j);

			if(qid.length() > 2) {
				qid = qid.substr(2, qid.length() - 2);
			}

			LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QuerySameQuestionAnswerLadyList( "
								"tid : %d, "
								"[获取回答过注册问题有共同答案的女士Id列表], "
								"qid : %s, "
								"aid : %s "
								")",
								(int)syscall(SYS_gettid),
								qid.c_str(),
								aid.c_str()
								);

			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND aid = %s;",
						pSiteId,
						qid.c_str(),
						aid.c_str()
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
									"MatchServer::QuerySameQuestionAnswerLadyList( "
									"tid : %d, "
									"[获取回答过注册问题有共同答案的女士Id列表], "
									"Count iQueryTime : %u m, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
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
						"WHERE qid = %s AND aid = %s "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						qid.c_str(),
						aid.c_str(),
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"MatchServer::QuerySameQuestionAnswerLadyList( "
									"tid : %d, "
									"[获取回答过注册问题有共同答案的女士Id列表], "
									"Query iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iRow2 : %d, "
									"iColumn2 : %d, "
									"iLadyCount : %d, "
									"iLadyIndex : %d, "
									"womanidMap.size() : %d "
									")",
									(int)syscall(SYS_gettid),
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
								"MatchServer::QuerySameQuestionAnswerLadyList( "
								"tid : %d, "
								"[获取回答过注册问题有共同答案的女士Id列表], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								iMax
								);
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QuerySameQuestionAnswerLadyList( "
								"tid : %d, "
								"[获取回答过注册问题有共同答案的女士Id列表], "
								"Double Check iQueryTime : %u ms, "
								"iQueryIndex : %d, "
								"iSingleQueryTime : %u ms "
								")",
								(int)syscall(SYS_gettid),
								iQueryTime,
								iQueryIndex,
								iSingleQueryTime
								);

			// 单词请求是否大于30ms
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}

			i = posQid + 1;
		    posQid = qpids.find(",", i);

			j = posAid + 1;
		    posAid = aids.find(",", j);

			iCount++;
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
	AddHandleTime(iHandleTime);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::QuerySameQuestionAnswerLadyList( "
			"tid : %d, "
			"[获取回答过注册问题有共同答案的女士Id列表], "
			"qids : %s, "
			"aids : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d, "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			pQids,
			pAids,
			pSiteId,
			pLimit,
			iQueryIndex,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

/**
 * 6.获取回答过注册问题有共同答案的女士Id列表
 * @param pQids		问题Id列表(逗号隔开, 最多:7个)
 * @param pSiteId	当前站点Id
 * @param pLimit	最大返回记录数目(默认:4, 最大:30)
 */
bool MatchServer::QuerySameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pQids,
		const char* pSiteId,
		const char* pLimit,
		int iQueryIndex
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"MatchServer::QuerySameQuestionLadyList( "
			"tid : %d, "
			"[获取回答过注册问题有共同答案的女士Id列表], "
			"qids : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			pQids,
			pSiteId,
			pLimit,
			iQueryIndex
			);

	// 执行查询
	char sql[1024] = {'\0'};

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bool bResult = false;
	int iCount = 0;

	if( pQids != NULL ) {
		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		bool bEnougthLady = false;

		string qpids = pQids;
		qpids += ",";
		string qid;

	    string::size_type posQid;

	    int i = 0;
	    posQid = qpids.find(",", i);

		while( posQid != string::npos && iCount < 7 ) {
			iSingleQueryTime = GetTickCount();

			qid = qpids.substr(i, posQid - i);

			if(qid.length() > 2) {
				qid = qid.substr(2, qid.length() - 2);
			}

			LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QuerySameQuestionLadyList( "
								"tid : %d, "
								"[获取回答过注册问题有共同答案的女士Id列表], "
								"qid : %s "
								")",
								(int)syscall(SYS_gettid),
								qid.c_str()
								);

			if( !bEnougthLady ) {
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s"
						";",
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
									"MatchServer::QuerySameQuestionLadyList( "
									"tid : %d, "
									"[获取回答过注册问题有共同答案的女士Id列表], "
									"Count iQueryTime : %u m, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
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
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"MatchServer::QuerySameQuestionLadyList( "
									"tid : %d, "
									"[获取回答过注册问题有共同答案的女士Id列表], "
									"Query iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iRow2 : %d, "
									"iColumn2 : %d, "
									"iLadyCount : %d, "
									"iLadyIndex : %d, "
									"womanidMap.size() : %d "
									")",
									(int)syscall(SYS_gettid),
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
								"MatchServer::QuerySameQuestionLadyList( "
								"tid : %d, "
								"[获取回答过注册问题有共同答案的女士Id列表], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								iMax
								);
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			LogManager::GetLogManager()->Log(
								LOG_STAT,
								"MatchServer::QuerySameQuestionLadyList( "
								"tid : %d, "
								"[获取回答过注册问题有共同答案的女士Id列表], "
								"Double Check iQueryTime : %u ms, "
								"iQueryIndex : %d, "
								"iSingleQueryTime : %u ms "
								")",
								(int)syscall(SYS_gettid),
								iQueryTime,
								iQueryIndex,
								iSingleQueryTime
								);

			// 单词请求是否大于30ms
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}

			i = posQid + 1;
		    posQid = qpids.find(",", i);

			iCount++;
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
	AddHandleTime(iHandleTime);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"MatchServer::QuerySameQuestionLadyList( "
			"tid : %d, "
			"[获取回答过注册问题有共同答案的女士Id列表], "
			"qids : %s, "
			"siteid : %s, "
			"limit : %s, "
			"iQueryIndex : %d, "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			pQids,
			pSiteId,
			pLimit,
			iQueryIndex,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}
