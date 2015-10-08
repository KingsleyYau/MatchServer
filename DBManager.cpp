/*
 * DBManager.cpp
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#include "DBManager.h"

class SyncRunnable : public KRunnable {
public:
	SyncRunnable(DBManager* pDBManager) {
		mpDBManager = pDBManager;
	}
	virtual ~SyncRunnable() {
	}
protected:
	void onRun() {
		while( true ) {
			mpDBManager->SyncDataFromDataBase();
			sleep(30);
		}
	}
	DBManager* mpDBManager;
};

DBManager::DBManager() {
	// TODO Auto-generated constructor stub
	miQueryIndex = 0;
	miMaxMemoryCopy = 1;
	miLastManRecordId = 0;
	miLastLadyRecordId = 0;

	mpSyncRunnable = new SyncRunnable(this);

	sqlite3_config(SQLITE_CONFIG_MEMSTATUS, false);
}

DBManager::~DBManager() {
	// TODO Auto-generated destructor stub
	mSyncThread.stop();

	if( mpSyncRunnable != NULL ) {
		delete mpSyncRunnable;
	}
}

bool DBManager::Init(int iMaxMemoryCopy, bool addTestData) {
	miMaxMemoryCopy = iMaxMemoryCopy;
	mdbs = new sqlite3*[iMaxMemoryCopy];

	bool bFlag = false;
	int ret;

	for( int i = 0; i < miMaxMemoryCopy; i++ ) {
		ret = sqlite3_open(":memory:", &mdbs[i]);
		if( ret == SQLITE_OK && CreateTable(mdbs[i]) ) {
			ExecSQL(mdbs[i], "PRAGMA synchronous = OFF;", 0);
			if( addTestData ) {
				if( InitTestData(mdbs[i]) ) {
					bFlag = true;
				}
			} else {
				bFlag = true;
			}
		}
	}

	// Start sync thread
//	mSyncThread.start(mpSyncRunnable);

	return bFlag;
}

bool DBManager::Query(char* sql, char*** result, int* iRow, int* iColumn) {
	int index;
	char *msg = NULL;
	bool bFlag = true;

	mIndexMutex.lock();
	index = miQueryIndex++;
	if( miQueryIndex == miMaxMemoryCopy ) {
		miQueryIndex = 0;
	}
	mIndexMutex.unlock();

	sqlite3* db = mdbs[index];
	bFlag = QuerySQL( db, sql, result, iRow, iColumn, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not select table, msg : %s \n", msg);
		bFlag = false;
		sqlite3_free(msg);
		msg = NULL;
	}

	return bFlag;
}

void DBManager::FinishQuery(char** result) {
	// free memory
	sqlite3_free_table(result);
}

void DBManager::SyncDataFromDataBase() {
	// load data from database as mysql
	timeval tStart;
	timeval tEnd;

	char *msg = NULL;
	char sql[2048] = {'\0'};
	char ids[64] = {'\0'};

	sprintf(sql,
			"SELECT ID, MANID, QID, AID FROM MAN WHERE ID > %d"
			";",
			miLastManRecordId
	);

	sprintf(sql,
			"SELECT ID, LADYID, QID, AID FROM LADY WHERE ID > %d"
			";",
			miLastLadyRecordId
	);

	sqlite3_stmt *stmtMan;
	sqlite3_stmt *stmtLady;

	// update memory
	gettimeofday(&tStart, NULL);
	for( int i = 0; i < miMaxMemoryCopy; i++ ) {
		ExecSQL( mdbs[i], "BEGIN;", NULL );
		sprintf(sql, "INSERT INTO MAN(`MANID`, `QID`, `AID`) VALUES(?, ?, ?)");
		sqlite3_prepare_v2(mdbs[i], sql, strlen(sql), &stmtMan, 0);

		sprintf(sql, "INSERT INTO LADY(`LADYID`, `QID`, `AID`) VALUES(?, ?, ?)");
		sqlite3_prepare_v2(mdbs[i], sql, strlen(sql), &stmtLady, 0);

		for( int id = miLastManRecordId; id < miLastManRecordId + 1000; id++ ) {
			for( int j = 0; j < 30; j++ ) {
				// insert man
				sprintf(ids, "man-%d", id);
//				sprintf(sql, "INSERT INTO MAN(`MANID`, `QID`, `AID`) VALUES('%s', %d, %d);",
//						ids,
//						j,
//						1
//						);
//				ExecSQL( mdbs[i], sql, &msg );
				sqlite3_reset(stmtMan);
				sqlite3_bind_text(stmtMan, 1, ids, strlen(ids), NULL);
				sqlite3_bind_int(stmtMan, 2, j);
				sqlite3_bind_int(stmtMan, 3, 1);
				sqlite3_step(stmtMan);
				miLastManRecordId++;

				sprintf(ids, "lady-%d", id);
//				sprintf(sql, "INSERT INTO LADY(`LADYID`, `QID`, `AID`) VALUES('%s', %d, %d);",
//						ids,
//						j,
//						1
//						);
//				ExecSQL( mdbs[i], sql, NULL );
				sqlite3_reset(stmtLady);
				sqlite3_bind_text(stmtLady, 1, ids, strlen(ids), NULL);
				sqlite3_bind_int(stmtLady, 2, j);
				sqlite3_bind_int(stmtLady, 3, 1);
				sqlite3_step(stmtLady);
				miLastLadyRecordId++;

				if( miLastManRecordId % 20 == 0 ) {
					usleep(100);
				}
			}
		}
		sqlite3_finalize(stmtMan);
		sqlite3_finalize(stmtLady);
		ExecSQL( mdbs[i], "COMMIT;", NULL );
	}
	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));

	miLastManRecordId += 1000;

	printf("# DBManager::SyncDataFromDataBase( 插入60000条记录用时 : %ldus ) \n\n", usec);
}

bool DBManager::DumpDB(sqlite3 *db) {
	char sql[2048] = {'\0'};
	char *msg = NULL;

	printf("# DumpDB() \n");
	sprintf(sql,
			"ATTACH DATABASE 'original.db' AS ORIGINAL"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not attach database, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	ExecSQL( db, "BEGIN TRANSACTION;", &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not attach database, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	sprintf(sql,
			"INSERT INTO MAN SELECT * FROM ORIGINAL.MAN"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not insert into man select * from original.man, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	sprintf(sql,
			"INSERT INTO LADY SELECT * FROM ORIGINAL.LADY"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not insert into lady select * from original.lady, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	ExecSQL( db, "COMMIT TRANSACTION;", &msg );
	if( msg != NULL ) {
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	return true;
}

bool DBManager::CreateTable(sqlite3 *db) {
	char sql[2048] = {'\0'};
	char *msg = NULL;

	// 建男士表
	sprintf(sql,
			"CREATE TABLE MAN("
						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"MANID VARCHAR(64),"
						"QID INT,"
						"AID INT"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table MAN ok \n");

	// 建男士表索引(MANID, QID, AID)
	sprintf(sql,
			"CREATE INDEX MANINDEX_MANID_QID_AID "
			"ON MAN (MANID, QID, AID)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建男士表索引(QID, AID)
	sprintf(sql,
			"CREATE INDEX MANINDEX_QID_AID "
			"ON MAN (QID, AID)"
			";"
		);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table MAN index ok \n");

	// 建女士表
	sprintf(sql,
			"CREATE TABLE LADY("
						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"LADYID VARCHAR(64), "
						"QID INT, "
						"AID INT"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table lady, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table LADY ok \n");

	// 建女士表索引(QID, AID)
	sprintf(sql,
			"CREATE INDEX LADYINDEX_QID_AID "
			"ON LADY (QID, AID)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table lady index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士表索引(LADYID, QID, AID)
	sprintf(sql,
			"CREATE INDEX LADYINDEX_LADYID_QID_AID "
			"ON LADY (LADYID, QID, AID)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table lady index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table LADY index ok \n");

	return true;
}

bool DBManager::InitTestData(sqlite3 *db) {
	char sql[2048] = {'\0'};
	char id[64] = {'\0'};
	char idlady[64] = {'\0'};
	char qid[64] = {'\0'};
	char aid[64] = {'\0'};

	char *msg = NULL;
	timeval tStart;
	timeval tEnd;
	bool bFlag = false;

	sqlite3_stmt *stmtMan;
	sqlite3_stmt *stmtLady;

	bFlag = ExecSQL( db, "BEGIN;", &msg );

	sprintf(sql, "INSERT INTO MAN(`MANID`, `QID`, `AID`) VALUES(?, ?, ?)");
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmtMan, 0);

	sprintf(sql, "INSERT INTO LADY(`LADYID`, `QID`, `AID`) VALUES(?, ?, ?)");
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmtLady, 0);

	// 插入表
	gettimeofday(&tStart, NULL);
	for( int i = 0; i < 40000; i++ ) {

		sprintf(id, "man-%d", i);
		sprintf(idlady, "lady-%d", i);

		for( int j = 0; j < 30; j++ ) {
			sprintf(qid, "%d", j);
			int r = rand() % 30;
			sprintf(aid, "%d", r);

			// insert man
//			sprintf(sql, "INSERT INTO MAN(`MANID`, `QID`, `AID`) VALUES('%s', %d, %d);",
//					id,
//					j,
//					r
//					);
//
//			bFlag = ExecSQL( db, sql, NULL );
			sqlite3_reset(stmtMan);
			sqlite3_bind_text(stmtMan, 1, id, strlen(id), NULL);
			sqlite3_bind_int(stmtMan, 2, j);
			sqlite3_bind_int(stmtMan, 3, r);
			sqlite3_step(stmtMan);
			miLastManRecordId++;

			// insert lady
			r = rand() % 30;
			sprintf(aid, "%d", r);

//			sprintf(sql, "INSERT INTO LADY(`LADYID`, `QID`, `AID`) VALUES('%s', %d, %d);",
//					id,
//					j,
//					r
//					);
//			bFlag = ExecSQL( db, sql, NULL );
			sqlite3_reset(stmtLady);
			sqlite3_bind_text(stmtLady, 1, idlady, strlen(idlady), NULL);
			sqlite3_bind_int(stmtLady, 2, j);
			sqlite3_bind_int(stmtLady, 3, r);
			sqlite3_step(stmtLady);
			miLastLadyRecordId++;

			if( miLastLadyRecordId % 20 == 0 ) {
				usleep(10);
			}
		}
	}
	sqlite3_finalize(stmtMan);
	sqlite3_finalize(stmtLady);
	ExecSQL( db, "COMMIT;", NULL );

	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
	printf("# Insert table ok time: %ldus \n", usec);
	printf("# 插入240万条记录时间: %lds \n\n", usec / 1000 / 1000);

	return true;
}

bool DBManager::ExecSQL(sqlite3 *db, char* sql, char** msg) {
	int ret;
	do {
		ret = sqlite3_exec( db, sql, NULL, NULL, msg );
		if( ret == SQLITE_BUSY ) {
			printf("# DBManager::ExecSQL( ret == SQLITE_BUSY )");
			if ( msg != NULL ) {
				sqlite3_free(msg);
				msg= NULL;
			}
			sleep(1);
		}
		if( ret != SQLITE_OK ) {
			printf("# DBManager::ExecSQL( ret : %d )", ret);
		}
	} while( ret == SQLITE_BUSY );

	return ( ret == SQLITE_OK );
}
bool DBManager::QuerySQL(sqlite3 *db, char* sql, char*** result, int* iRow, int* iColumn, char** msg) {
	int ret;
	do {
		ret = sqlite3_get_table( db, sql, result, iRow, iColumn, msg );
		if( ret == SQLITE_BUSY ) {
			printf("# DBManager::QuerySQL( ret == SQLITE_BUSY ) \n");
			if ( msg != NULL ) {
				sqlite3_free(msg);
				msg= NULL;
			}
			sleep(1);
		}

		if( ret != SQLITE_OK ) {
			printf("# DBManager::QuerySQL( ret : %d ) \n", ret);
		}
	} while( ret == SQLITE_BUSY );

	return ( ret == SQLITE_OK );
}
