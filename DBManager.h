/*
 * DBManager.h
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#ifndef DBMANAGER_H_
#define DBMANAGER_H_

#include "KThread.h"

using namespace std;
#include <string.h>

#include <sqlite3.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

class SyncRunnable;
class DBManager {
public:
	DBManager();
	virtual ~DBManager();

	bool Init(int iMaxMemoryCopy, bool addTestData = false);

	bool Query(char* sql, char** result, int* iRow, int* iColumn);
	void FinishQuery(char** result);

	void SyncDataFromDataBase();

private:
	int miQueryIndex;
	int miMaxMemoryCopy;
	sqlite3** mdbs;
	int miLastManRecordId;
	int miLastLadyRecordId;

	KMutex mIndexMutex;
	KMutex mBusyMutex;
	KThread mSyncThread;
	SyncRunnable* mpSyncRunnable;

	bool CreateTable(sqlite3 *db);
	bool DumpDB(sqlite3 *db);
	bool InitTestData(sqlite3 *db);

	bool ExecSQL(sqlite3 *db, char* sql, char** msg);
	bool QuerySQL(sqlite3 *db, char* sql, char** result, int* iRow, int* iColumn, char** msg);
};

#endif /* DBMANAGER_H_ */
