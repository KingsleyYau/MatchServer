/*
 * DBManager.h
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#ifndef DBMANAGER_H_
#define DBMANAGER_H_

using namespace std;
#include <string.h>

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "KThread.h"
#include "DBSpool.hpp"
#include "LogManager.h"
#include "TimeProc.hpp"

class SyncRunnable;

typedef struct DBStruct {
	DBStruct() {
		mHost = "";
		mPort = -1;
		mUser = "";
		mPasswd = "";
		mDbName = "";
		miMaxDatabaseThread = 0;
		miSiteId = -1;
	}

	string mHost;
	short mPort;
	string mUser;
	string mPasswd;
	string mDbName;
	int miMaxDatabaseThread;
	int miSiteId;
} DBSTRUCT;

class DBManager;
class DBManagerListener {
public:
	virtual ~DBManagerListener(){};
	virtual void OnSyncFinish(DBManager* pDBManager) = 0;
};
class DBManager {
public:
	DBManager();
	virtual ~DBManager();

	bool Init(
			int iMaxMemoryCopy,
			const DBSTRUCT& dbQA,
			const DBSTRUCT* dbOnline,
			int iDbOnlineCount,
			bool addTestData = false
			);
	void SetDBManagerListener(DBManagerListener *pDBManagerListener);

	bool Query(char* sql, char*** result, int* iRow, int* iColumn, int index = 0);
	void FinishQuery(char** result);

	void SetSyncTime(int second);
	void SetSyncOnlineTime(int second);

	void SyncOnlineLady(int index);
	void SyncManAndLady();
	void SyncDataFromDataBase();

	void Status();

	void SyncForce();
	void HandleSyncDatabase();

private:
	DBManagerListener *mpDBManagerListener;
	int miQueryIndex;
	int miMaxMemoryCopy;
	sqlite3** mdbs;

	int miLastManRecordId;
	int miLastLadyRecordId;
	long long miLastUpdateMan;
	long long miLastUpdateLady;

	int miSyncTime;
	int miSyncOnlineTime;
	bool mbSyncForce;
	KMutex mSyncMutex;

	char* mpSqliteHeapBuffer;
	char* mpSqlitePageBuffer;

	DBSpool mDBSpool;
	DBSpool mDBSpoolOnline[4];
	int miSiteId[4];
	int miDbOnlineCount;

	KMutex mIndexMutex;
	KMutex mBusyMutex;
	KThread mSyncThread;
	SyncRunnable* mpSyncRunnable;

	bool CreateTable(sqlite3 *db);
	bool DumpDB(sqlite3 *db);
	bool InitTestData(sqlite3 *db);

	bool InsertManFromDataBase(sqlite3_stmt *stmtMan, MYSQL_ROW &row, int iFields);
	bool InsertLadyFromDataBase(sqlite3_stmt *stmtLady, MYSQL_ROW &row, int iFields);
	bool InsertOnlineLadyFromDataBase(sqlite3_stmt *stmtOnlineLady, MYSQL_ROW &row, int iFields);

	bool ExecSQL(sqlite3 *db, const char* sql, char** msg);
	bool QuerySQL(sqlite3 *db, const char* sql, char*** result, int* iRow, int* iColumn, char** msg);
};

#endif /* DBMANAGER_H_ */
