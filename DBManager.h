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

class SyncRunnable;
class DBManager {
public:
	DBManager();
	virtual ~DBManager();

	bool Init(int iMaxMemoryCopy, bool addTestData = false);
	bool InitSyncDataBase(
			int iMaxThread,
			const char* pcHost,
			short shPort,
			const char* pcDBname,
	        const char* pcUser,
	        const char* pcPasswd
	        );

	bool Query(char* sql, char*** result, int* iRow, int* iColumn, int index = -1);
	void FinishQuery(char** result);

	int GetSyncDataTime();
	void SetSyncDataTime(int second);
	void SyncDataFromDataBase();
	void Status();

	void SyncForce();
	void HandleSyncDatabase();

private:
	int miQueryIndex;
	int miMaxMemoryCopy;
	sqlite3** mdbs;

	int miLastManRecordId;
	int miLastLadyRecordId;
	long long miLastUpdateMan;
	long long miLastUpdateLady;

	int mSyncDataTime;
	bool mbSyncForce;
	KMutex mSyncMutex;

	char* mpSqliteHeapBuffer;
	char* mpSqlitePageBuffer;

	DBSpool mDBSpool;
	KMutex mIndexMutex;
	KMutex mBusyMutex;
	KThread mSyncThread;
	SyncRunnable* mpSyncRunnable;

	bool CreateTable(sqlite3 *db);
	bool DumpDB(sqlite3 *db);
	bool InitTestData(sqlite3 *db);

	bool InsertManFromDataBase(sqlite3_stmt *stmtMan, MYSQL_ROW &row, int iFields);
	bool InsertLadyFromDataBase(sqlite3_stmt *stmtLady, MYSQL_ROW &row, int iFields);

	bool ExecSQL(sqlite3 *db, char* sql, char** msg);
	bool QuerySQL(sqlite3 *db, char* sql, char*** result, int* iRow, int* iColumn, char** msg);
};

#endif /* DBMANAGER_H_ */
