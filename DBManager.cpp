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
		mpDBManager->HandleSyncDatabase();
	}
	DBManager* mpDBManager;
};

DBManager::DBManager() {
	// TODO Auto-generated constructor stub
	miQueryIndex = 0;
	miMaxMemoryCopy = 1;

	miLastManRecordId = 0;
	miLastLadyRecordId = 0;
	miLastUpdateMan = 0;
	miLastUpdateLady = 0;;

	mbSyncForce = false;
	mSyncDataTime = 30 * 60;
	mSyncOnlineLadyTime = 1 * 60;
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
			ExecSQL(mdbs[i], (char*)"PRAGMA synchronous = OFF;", 0);
			if( addTestData ) {
				if( InitTestData(mdbs[i]) ) {
					bFlag = true;
				}
			} else {
				bFlag = true;
			}
		}
	}
	printf("# DBManager initialize OK. \n");
	return bFlag;
}

bool DBManager::InitSyncDataBase(
			int iMaxThread,

			const char* pcHost,
			short shPort,
			const char* pcDBname,
	        const char* pcUser,
	        const char* pcPasswd,

			const char* pcHostOnline,
			short shPortOnline,
			const char* pcDBnameOnline,
	        const char* pcUserOnline,
	        const char* pcPasswdOnline
	        ) {
	printf("# DBManager synchronizing... \n");

	bool bFlag = false;

	bFlag = mDBSpool.SetConnection(iMaxThread);
	bFlag = bFlag && mDBSpool.SetDBparm(
			pcHost,
			shPort,
			pcDBname,
			pcUser,
			pcPasswd
			);
	bFlag = bFlag && mDBSpool.Connect();

	bFlag = mDBSpoolOnline.SetConnection(iMaxThread);
	bFlag = bFlag && mDBSpoolOnline.SetDBparm(
			pcHostOnline,
			shPortOnline,
			pcDBnameOnline,
			pcUserOnline,
			pcPasswdOnline
			);
	bFlag = bFlag && mDBSpoolOnline.Connect();

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"DBManager::InitSyncDataBase( "
			"pcHost : %s, "
			"shPort : %d, "
			"pcDBname : %s, "
			"pcUser : %s, "
			"pcPasswd : %s, "
			"pcHostOnline : %s, "
			"shPortOnline : %d, "
			"pcDBnameOnline : %s, "
			"pcUserOnline : %s, "
			"pcPasswdOnline : %s, "
			"bFlag : %s "
			")",
			pcHost,
			shPort,
			pcDBname,
			pcUser,
			pcPasswd,
			pcHostOnline,
			shPortOnline,
			pcDBnameOnline,
			pcUserOnline,
			pcPasswdOnline,
			bFlag?"true":"false"
			);

	if( bFlag ) {
		SyncDataFromDataBase();

		// Start sync thread
		mSyncThread.start(mpSyncRunnable);
	}

	printf("# DBManager synchronizie OK. \n");
	return bFlag;
}

bool DBManager::Query(char* sql, char*** result, int* iRow, int* iColumn, int index) {
	char *msg = NULL;
	bool bFlag = true;

	int i = index % miMaxMemoryCopy;

	sqlite3* db = mdbs[i];
	bFlag = QuerySQL( db, sql, result, iRow, iColumn, NULL );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
								LOG_ERR_USER,
								"DBManager::Query( "
								"tid : %d, "
								"Could not select table, msg : %s "
								")",
								(int)syscall(SYS_gettid),
								msg
								);
//		fprintf(stderr, "# Could not select table, msg : %s \n", msg);
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

void DBManager::SyncOnlineLady() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncOnlineLady( "
			"tid : %d, "
			"start "
			")",
			(int)syscall(SYS_gettid)
			);

	// 同步在线表
	unsigned int iHandleTime = GetTickCount();
	char sql[1024] = {'\0'};

	sprintf(sql,
			"SELECT `id`, `womanid` FROM online_woman "
			"WHERE status = 1 AND hide = 0 AND binding IN (1, 2)"
			";"
	);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncOnlineLady( "
			"tid : %d, "
			"sql : %s "
			")",
			(int)syscall(SYS_gettid),
			sql
			);

	sqlite3_stmt **stmtOnlineLady = new sqlite3_stmt*[miMaxMemoryCopy];

	MYSQL_RES* pSQLRes = NULL;
	short shIdt = 0;
	int iRows = 0;
	if (SQL_TYPE_SELECT == mDBSpoolOnline.ExecuteSQL(sql, &pSQLRes, shIdt, iRows)
		&& iRows > 0) {
		int iFields = mysql_num_fields(pSQLRes);
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"DBManager::SyncOnlineLady( "
				"tid : %d, "
				"sql : %s, "
				"iRows : %d, "
				"iFields : %d "
				")",
				(int)syscall(SYS_gettid),
				sql,
				iRows,
				iFields
				);
		if (iFields > 0) {
			MYSQL_FIELD* fields;
			MYSQL_ROW row;
			fields = mysql_fetch_fields(pSQLRes);

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				ExecSQL( mdbs[i], "BEGIN;", NULL );

				// 删掉旧的数据
//				ExecSQL( mdbs[i], "DELETE FROM online_woman;", NULL );

				sprintf(sql, "INSERT INTO online_woman("
						"`id`, "
						"`womanid` "
						") "
						"VALUES("
						"?, "
						"?"
						")"
						);
				sqlite3_prepare_v2(mdbs[i], sql, strlen(sql), &(stmtOnlineLady[i]), 0);
			}

			miLastOnlineLadyRecordId = 0;

			for (int i = 0; i < iRows; i++) {
				if ((row = mysql_fetch_row(pSQLRes)) == NULL) {
					break;
				}

				for( int j = 0; j < miMaxMemoryCopy; j++ ) {
					// insert online_woman
					bool bFlag = InsertOnlineLadyFromDataBase(stmtOnlineLady[j], row, iFields);
					if( !bFlag ) {
						string value = "[";
						for( int j = 0; j < iFields; j++ ) {
							value += row[j];
							value += ",";
						}
						if( value.length() > 1 ) {
							value = value.substr(0, value.length() - 1);
						}
						value += "]";
						LogManager::GetLogManager()->Log(
								LOG_ERR_USER,
								"DBManager::SyncOnlineLady( "
								"tid : %d, "
								"InsertOnlineLadyFromDataBase fail, "
								"row : %d, "
								"value : %s "
								")",
								(int)syscall(SYS_gettid),
								i,
								value.c_str()
								);
					}
				}

				miLastOnlineLadyRecordId++;
			}

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				sqlite3_finalize(stmtOnlineLady[i]);
				ExecSQL( mdbs[i], "COMMIT;", NULL );
			}
		}
	}

	mDBSpoolOnline.ReleaseConnection(shIdt);
	if( stmtOnlineLady ) {
		delete stmtOnlineLady;
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncOnlineLady( "
			"tid : %d, "
			"iHandleTime : %u ms "
			"end "
			")",
			(int)syscall(SYS_gettid),
			iHandleTime
			);
}

void DBManager::SyncManAndLady() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncManAndLady( "
			"tid : %d, "
			"start "
			")",
			(int)syscall(SYS_gettid)
			);

	// 同步在线女士
	unsigned int iHandleTime = GetTickCount();
	char sql[1024] = {'\0'};

	sqlite3_stmt **stmtMan = new sqlite3_stmt*[miMaxMemoryCopy];
	sqlite3_stmt **stmtLady = new sqlite3_stmt*[miMaxMemoryCopy];

	sprintf(sql,
			"SELECT `id`, `q_id`, `question_status`, `manid`, `answer_id`, UNIX_TIMESTAMP(`last_update`), `siteid` FROM mq_man_answer WHERE UNIX_TIMESTAMP(`last_update`) > %lld"
			";",
			miLastUpdateMan
	);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncManAndLady( "
			"tid : %d, "
			"sql : %s "
			")",
			(int)syscall(SYS_gettid),
			sql
			);

	MYSQL_RES* pSQLRes = NULL;
	short shIdt = 0;
	int iRows = 0;
	if (SQL_TYPE_SELECT == mDBSpool.ExecuteSQL(sql, &pSQLRes, shIdt, iRows)
		&& iRows > 0) {
		int iFields = mysql_num_fields(pSQLRes);
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"DBManager::SyncManAndLady( "
				"tid : %d, "
				"sql : %s, "
				"iRows : %d, "
				"iFields : %d "
				")",
				(int)syscall(SYS_gettid),
				sql,
				iRows,
				iFields
				);
		if (iFields > 0) {
			MYSQL_FIELD* fields;
			MYSQL_ROW row;
			fields = mysql_fetch_fields(pSQLRes);

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				ExecSQL( mdbs[i], "BEGIN;", NULL );
				sprintf(sql, "REPLACE INTO mq_man_answer("
						"`id`, "
						"`qid`, "
						"`question_status`, "
						"`manid`, "
						"`aid`, "
						"`siteid`"
						") "
						"VALUES("
						"?, "
						"?, "
						"?, "
						"?, "
						"?, "
						"?"
						")"
						);
				sqlite3_prepare_v2(mdbs[i], sql, strlen(sql), &(stmtMan[i]), 0);
			}

			for (int i = 0; i < iRows; i++) {
				if ((row = mysql_fetch_row(pSQLRes)) == NULL) {
					break;
				}

				if( row[0] ) {
					int temp = atoi(row[0]);
					miLastManRecordId = temp;
				}

				for( int j = 0; j < miMaxMemoryCopy; j++ ) {
					// insert mq_man_answer
					bool bFlag = InsertManFromDataBase(stmtMan[j], row, iFields);
					if( !bFlag ) {
						string value = "[";
						for( int j = 0; j < iFields; j++ ) {
							value += row[j];
							value += ",";
						}
						if( value.length() > 1 ) {
							value = value.substr(0, value.length() - 1);
						}
						value += "]";
						LogManager::GetLogManager()->Log(
								LOG_ERR_USER,
								"DBManager::SyncManAndLady( "
								"tid : %d, "
								"InsertManFromDataBase fail, "
								"row : %d, "
								"value : %s "
								")",
								(int)syscall(SYS_gettid),
								i,
								value.c_str()
								);
					}
				}
			}

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				sqlite3_finalize(stmtMan[i]);
				ExecSQL( mdbs[i], "COMMIT;", NULL );
			}

		}
	}
	mDBSpool.ReleaseConnection(shIdt);

	sprintf(sql,
			"SELECT `id`,`q_id`, `question_status`, `womanid`, `answer_id`, UNIX_TIMESTAMP(`last_update`), `siteid` FROM mq_woman_answer WHERE UNIX_TIMESTAMP(`last_update`) > %lld"
			";",
			miLastUpdateLady
	);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncManAndLady( "
			"tid : %d, "
			"sql : %s "
			")",
			(int)syscall(SYS_gettid),
			sql
			);
	if (SQL_TYPE_SELECT == mDBSpool.ExecuteSQL(sql, &pSQLRes, shIdt, iRows)
		&& iRows > 0) {
		int iFields = mysql_num_fields(pSQLRes);
		LogManager::GetLogManager()->Log(
						LOG_MSG,
						"DBManager::SyncManAndLady( "
						"tid : %d, "
						"sql : %s, "
						"iRows : %d, "
						"iFields : %d "
						")",
						(int)syscall(SYS_gettid),
						sql,
						iRows,
						iFields
						);
		if (iFields > 0) {
			MYSQL_FIELD* fields;
			MYSQL_ROW row;
			fields = mysql_fetch_fields(pSQLRes);

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				ExecSQL( mdbs[i], (char*)"BEGIN;", NULL );
				sprintf(sql, (char*)"REPLACE INTO mq_woman_answer("
						"`id`, "
						"`qid`, "
						"`question_status`, "
						"`womanid`, "
						"`aid`, "
						"`siteid`"
						") "
						"VALUES("
						"?, "
						"?, "
						"?, "
						"?, "
						"?, "
						"?"
						")");
				sqlite3_prepare_v2(mdbs[i], sql, strlen(sql), &(stmtLady[i]), 0);
			}

			for (int i = 0; i < iRows; i++) {
				if ((row = mysql_fetch_row(pSQLRes)) == NULL) {
					break;
				}

				if( row[0] ) {
					miLastLadyRecordId = atoi(row[0]);
				}

				for( int j = 0; j < miMaxMemoryCopy; j++ ) {
					// insert lady
					bool bFlag = InsertLadyFromDataBase(stmtLady[j], row, iFields);
					if( !bFlag ) {
						string value = "[";
						for( int j = 0; j < iFields; j++ ) {
							value += row[j];
							value += ",";
						}
						if( value.length() > 1 ) {
							value = value.substr(0, value.length() - 1);
						}
						value += "]";
						LogManager::GetLogManager()->Log(
								LOG_ERR_USER,
								"DBManager::SyncManAndLady( "
								"tid : %d, "
								"InsertLadyFromDataBase fail, "
								"row : %d, "
								"value : %s "
								")",
								(int)syscall(SYS_gettid),
								i,
								value.c_str()
								);
					}
				}
			}

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				sqlite3_finalize(stmtLady[i]);
				ExecSQL( mdbs[i], "COMMIT;", NULL );
			}
		}
	}
	mDBSpool.ReleaseConnection(shIdt);

	if( stmtMan ) {
		delete stmtMan;
	}

	if( stmtLady ) {
		delete stmtLady;
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncManAndLady( "
			"tid : %d, "
			"iHandleTime : %u ms "
			"end "
			")",
			(int)syscall(SYS_gettid),
			iHandleTime
			);
}

void DBManager::SyncDataFromDataBase() {
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncDataFromDataBase( "
			"tid : %d, "
			"start "
			")",
			(int)syscall(SYS_gettid)
			);

	unsigned int iHandleTime = GetTickCount();

	// load data from database as mysql
	// 同步在线女士
	SyncOnlineLady();

	// 同步男士女士问题
	SyncManAndLady();

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncDataFromDataBase( "
			"tid : %d, "
			"iHandleTime : %u ms "
			"end "
			")",
			(int)syscall(SYS_gettid),
			iHandleTime
			);
}

void DBManager::Status() {
	sqlite3* db;
	int current;
	int highwater;

	for( int i = 0; i < miMaxMemoryCopy; i++ ) {
		db = mdbs[i];
		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_USED, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_LOOKASIDE_USED, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_HIT, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_LOOKASIDE_HIT, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_USED, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_CACHE_USED, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_SCHEMA_USED, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_SCHEMA_USED, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_STMT_USED, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_STMT_USED, current : %d, highwater : %d \n", i, current, highwater);


		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_HIT, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_CACHE_HIT, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_MISS, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_CACHE_MISS, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_CACHE_WRITE, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_CACHE_WRITE, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_DBSTATUS_DEFERRED_FKS, &current, &highwater, false);
		printf("# db[%d] SQLITE_DBSTATUS_DEFERRED_FKS, current : %d, highwater : %d \n", i, current, highwater);
	}

}

void DBManager::SetSyncDataTime(int second) {
	mSyncDataTime = second;
}

void DBManager::SetSyncOnlineLadyTime(int second) {
	mSyncOnlineLadyTime = second;
}

bool DBManager::CreateTable(sqlite3 *db) {
	char sql[2048] = {'\0'};
	char *msg = NULL;

	// 建男士表
	sprintf(sql,
			"CREATE TABLE mq_man_answer("
//						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"id INTEGER PRIMARY KEY,"
						"qid BIGINT,"
						"question_status INTEGER,"
						"manid TEXT,"
						"aid INTEGER,"
						"siteid INTEGER"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table mq_man_answer, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建男士表索引(manid, qid, aid)
	sprintf(sql,
			"CREATE INDEX manindex_manid_qid_aid "
			"ON mq_man_answer (manid, qid, aid)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table mq_man_answer index, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士表
	sprintf(sql,
			"CREATE TABLE mq_woman_answer("
//						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"id INTEGER PRIMARY KEY,"
						"qid BIGINT,"
						"question_status INTEGER,"
						"womanid TEXT,"
						"aid INTEGER,"
						"siteid INTEGER"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table mq_woman_answer, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士表索引(qid, aid, siteid, question_status)
	sprintf(sql,
			"CREATE INDEX womanindex_qid_aid_siteid_question_status "
			"ON mq_woman_answer (qid, aid, siteid, question_status)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table mq_woman_answer index, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士表索引(womanid, qid, aid)
	sprintf(sql,
			"CREATE UNIQUE INDEX womanindex_womanid_qid_aid_siteid_question_status "
			"ON mq_woman_answer (womanid, qid, aid, siteid, question_status)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table mq_woman_answer index, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士在线表
	sprintf(sql,
			"CREATE TABLE online_woman("
//						"recordid INTEGER PRIMARY KEY AUTOINCREMENT,"
						"id INTEGER PRIMARY KEY,"
						"womanid TEXT"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table online_woman, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士在线表索引(womanid)
	sprintf(sql,
			"CREATE UNIQUE INDEX online_woman_index_womanid "
			"ON online_woman (womanid)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		LogManager::GetLogManager()->Log(
				LOG_ERR_USER,
				"DBManager::CreateTable( "
				"tid : %d, "
				"sql : %s, "
				"Could not create table online_woman index, msg : %s "
				")",
				(int)syscall(SYS_gettid),
				sql,
				msg
				);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

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

	bFlag = ExecSQL( db, (char*)"BEGIN;", &msg );

	sprintf(sql, "INSERT INTO mq_man_answer(`id`, `qid`, `question_status`, `manid`, `aid`, `siteid`) VALUES(?, ?, 1, ?, ?, 1)");
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmtMan, 0);

	sprintf(sql, "INSERT INTO mq_woman_answer(`id`, `qid`, `question_status`, `womanid`, `aid`, `siteid`) VALUES(?, ?, 1, ?, ?, 1)");
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmtLady, 0);

	// 插入表
	gettimeofday(&tStart, NULL);
	for( int i = 0; i < 40000; i++ ) {

		sprintf(id, "CM%d", i);
		sprintf(idlady, "CM%d", i);

		for( int j = 0; j < 30; j++ ) {
			sprintf(qid, "%d", j);
			int r = rand() % 4;
			sprintf(aid, "%d", r);

			sqlite3_reset(stmtMan);
			sqlite3_bind_int(stmtMan, 1, miLastManRecordId);
			sqlite3_bind_int(stmtMan, 2, j);
			sqlite3_bind_text(stmtMan, 3, id, strlen(id), NULL);
			sqlite3_bind_int(stmtMan, 4, r);
			sqlite3_step(stmtMan);
			miLastManRecordId++;

			// insert lady
			r = rand() % 4;
			sprintf(aid, "%d", r);

			sqlite3_reset(stmtLady);
			sqlite3_bind_int(stmtLady, 1, miLastLadyRecordId);
			sqlite3_bind_int(stmtLady, 2, j);
			sqlite3_bind_text(stmtLady, 3, idlady, strlen(idlady), NULL);
			sqlite3_bind_int(stmtLady, 4, r);
			sqlite3_step(stmtLady);
			miLastLadyRecordId++;

//			if( miLastLadyRecordId % 60 == 0 ) {
//				usleep(10);
//			}
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

bool DBManager::ExecSQL(sqlite3 *db, const char* sql, char** msg) {
	int ret;
	do {
		ret = sqlite3_exec( db, sql, NULL, NULL, msg );
		if( ret == SQLITE_BUSY ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"DBManager::ExecSQL( "
					"tid : %d, "
					"ret == SQLITE_BUSY, "
					"sqlite3_errmsg() : %s, "
					"sql : %s "
					")",
					(int)syscall(SYS_gettid),
					sqlite3_errmsg(db),
					sql
					);

			if ( msg != NULL ) {
				sqlite3_free(msg);
				msg= NULL;
			}
			sleep(1);
		}
		if( ret != SQLITE_OK ) {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"DBManager::ExecSQL( "
					"tid : %d, "
					"ret != SQLITE_OK, "
					"ret : %d, "
					"sqlite3_errmsg() : %s, "
					"sql : %s "
					")",
					(int)syscall(SYS_gettid),
					ret,
					sqlite3_errmsg(db),
					sql
					);
//			printf("# DBManager::ExecSQL( ret != SQLITE_OK, ret : %d ) \n", ret);
		}
	} while( ret == SQLITE_BUSY );

	return ( ret == SQLITE_OK );
}
bool DBManager::QuerySQL(sqlite3 *db, const char* sql, char*** result, int* iRow, int* iColumn, char** msg) {
	int ret;
	do {
		ret = sqlite3_get_table( db, sql, result, iRow, iColumn, msg );
		if( ret == SQLITE_BUSY ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"DBManager::QuerySQL( "
					"tid : %d, "
					"ret == SQLITE_BUSY, "
					"sqlite3_errmsg() : %s, "
					"sql : %s "
					")",
					(int)syscall(SYS_gettid),
					sqlite3_errmsg(db),
					sql
					);
			if ( msg != NULL ) {
				sqlite3_free(msg);
				msg= NULL;
			}
			sleep(1);
		}

		if( ret != SQLITE_OK ) {
			LogManager::GetLogManager()->Log(
					LOG_ERR_USER,
					"DBManager::QuerySQL( "
					"tid : %d, "
					"ret != SQLITE_OK, "
					"ret : %d, "
					"sqlite3_errmsg() : %s, "
					"sql : %s "
					")",
					(int)syscall(SYS_gettid),
					ret,
					sqlite3_errmsg(db),
					sql
					);
		}
	} while( ret == SQLITE_BUSY );

	return ( ret == SQLITE_OK );
}

bool DBManager::InsertManFromDataBase(sqlite3_stmt *stmtMan, MYSQL_ROW &row, int iFields) {
	sqlite3_reset(stmtMan);

	if( iFields == 7 ) {
		// id
		if( row[0] ) {
			sqlite3_bind_int(stmtMan, 1, atoi(row[0]));
		} else {
			return false;
		}
		// q_id -> qid
		if( row[1] ) {
			string qid = row[1];
			qid = qid.substr(2, qid.length() - 2);
			sqlite3_bind_int64(stmtMan, 2, atoll(qid.c_str()));
		} else {
			return false;
		}
		// question_status
		if( row[2] ) {
			sqlite3_bind_int(stmtMan, 3, atoi(row[2]));
		} else {
			return false;
		}
		// manid
		if( row[3] ) {
			sqlite3_bind_text(stmtMan, 4, row[3], strlen(row[3]), NULL);
		} else {
			return false;
		}
		// answer_id
		if( row[4] ) {
			sqlite3_bind_int(stmtMan, 5, atoi(row[4]));
		} else {
			return false;
		}
		// last_update
		if( row[5] ) {
			int timestamp = atoll(row[5]);
			if( miLastUpdateMan < timestamp ) {
				miLastUpdateMan = timestamp;
			}
//			sqlite3_bind_text(stmtMan, 12, row[11], strlen(row[11]), NULL);
		} else {
			return false;
		}
		// siteid
		if( row[6] ) {
			sqlite3_bind_int(stmtMan, 6, atoi(row[6]));
		} else {
			return false;
		}

		sqlite3_step(stmtMan);

	} else {
		return false;
	}

	return true;
}

bool DBManager::InsertLadyFromDataBase(sqlite3_stmt *stmtLady, MYSQL_ROW &row, int iFields) {
	sqlite3_reset(stmtLady);

	if( iFields == 7 ) {
		// id
		if( row[0] ) {
			sqlite3_bind_int(stmtLady, 1, atoi(row[0]));
		} else {
			return false;
		}
		// q_id -> qid
		if( row[1] ) {
			string qid = row[1];
			qid = qid.substr(2, qid.length() - 2);
			sqlite3_bind_int64(stmtLady, 2, atoll(qid.c_str()));
		} else {
			return false;
		}
		// question_status
		if( row[2] ) {
			sqlite3_bind_int(stmtLady, 3, atoi(row[2]));
		} else {
			return false;
		}
		// womanid
		if( row[3] ) {
			sqlite3_bind_text(stmtLady, 4, row[3], strlen(row[3]), NULL);
		} else {
			return false;
		}
		// answer_id
		if( row[4] ) {
			sqlite3_bind_int(stmtLady, 5, atoi(row[4]));
		} else {
			return false;
		}
		// last_update
		if( row[5] ) {
			int timestamp = atoll(row[5]);
			if( miLastUpdateLady < timestamp ) {
				miLastUpdateLady = timestamp;
			}
//			sqlite3_bind_text(stmtLady, 12, row[11], strlen(row[11]), NULL);
		} else {
			return false;
		}
		// siteid
		if( row[6] ) {
			sqlite3_bind_int(stmtLady, 6, atoi(row[6]));
		} else {
			return false;
		}

		sqlite3_step(stmtLady);

	} else {
		return false;
	}

	return true;
}

bool DBManager::InsertOnlineLadyFromDataBase(sqlite3_stmt *stmtOnlineLady, MYSQL_ROW &row, int iFields) {
	sqlite3_reset(stmtOnlineLady);

	if( iFields == 2 ) {
		// id
		if( row[0] ) {
			sqlite3_bind_int(stmtOnlineLady, 1, atoi(row[0]));
		} else {
			return false;
		}
		// womanid
		if( row[1] ) {
			sqlite3_bind_text(stmtOnlineLady, 2, row[1], strlen(row[1]), NULL);
		} else {
			return false;
		}

		sqlite3_step(stmtOnlineLady);
	} else {
		return false;
	}

	return true;
}

void DBManager::SyncForce() {
	mSyncMutex.lock();
	mbSyncForce = true;
	mSyncMutex.unlock();
}

void DBManager::HandleSyncDatabase() {
	int i = 0;
	while( true ) {
		mSyncMutex.lock();
		if( mbSyncForce ) {
			mbSyncForce = false;
			mSyncMutex.unlock();

			SyncDataFromDataBase();

			i = 0;
		} else {
			mSyncMutex.unlock();

			if( i % mSyncOnlineLadyTime == 0 ) {
				// 同步在线女士
				SyncOnlineLady();
			}

			if( i % mSyncDataTime == 0 ) {
				// 同步男士女士问题
				SyncManAndLady();
				i = 0;
			}
		}
		i++;
		sleep(1);
	}
}

int DBManager::GetLastOnlineLadyRecordId() {
	return miLastOnlineLadyRecordId;
}
