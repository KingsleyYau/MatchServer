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
		int i = 0;
		while( true ) {
			mpDBManager->HandleSyncDatabase();
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
	miLastUpdateMan = 0;
	miLastUpdateLady = 0;;

	mbSyncForce = false;
	mSyncDataTime = 30 * 1000;
	mpSyncRunnable = new SyncRunnable(this);

	sqlite3_config(SQLITE_CONFIG_MEMSTATUS, false);

//	int size = 200 * 1024 * 1024;
//	mpSqliteHeapBuffer = new char[size];
//	sqlite3_config(SQLITE_CONFIG_HEAP, mpSqliteHeapBuffer, size, 2);
//
//	int size = 1024 * 1024 * 1024;
//	mpSqlitePageBuffer = new char[size];
//	sqlite3_config(SQLITE_CONFIG_PAGECACHE, mpSqlitePageBuffer, 4, 256 * 1024 * 1024);
//
//	sqlite3_config(SQLITE_CONFIG_LOOKASIDE, 1024, 1024);
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

	return bFlag;
}

bool DBManager::InitSyncDataBase(
			int iMaxThread,
			const char* pcHost,
			short shPort,
			const char* pcDBname,
	        const char* pcUser,
	        const char* pcPasswd
	        ) {
	bool bFlag = false;

	bFlag = mDBSpool.SetConnection(iMaxThread);
	bFlag = bFlag && mDBSpool.SetDBparm(pcHost, shPort, pcDBname,
					pcUser, pcPasswd);
	bFlag = bFlag && mDBSpool.Connect();

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::InitSyncDataBase( "
			"pcHost : %s, "
			"shPort : %d, "
			"pcDBname : %s, "
			"pcUser : %s, "
			"pcPasswd : %s, "
			"bFlag : %s "
			")",
			pcHost,
			shPort,
			pcDBname,
			pcUser,
			pcPasswd,
			bFlag?"true":"false"
			);

	if( bFlag ) {
		SyncDataFromDataBase();

		// Start sync thread
		mSyncThread.start(mpSyncRunnable);
	}

	return bFlag;
}

bool DBManager::Query(char* sql, char*** result, int* iRow, int* iColumn) {
	LogManager::GetLogManager()->Log(
							LOG_STAT, "DBManager::Query( "
							"tid : %d, "
							"sql : %s "
							")",
							(int)syscall(SYS_gettid),
							sql
							);
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
		LogManager::GetLogManager()->Log(
								LOG_STAT, "DBManager::Query( "
								"tid : %d, "
								"Could not select table, msg : %s "
								")",
								(int)syscall(SYS_gettid),
								msg
								);
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
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncDataFromDataBase( "
			"tid : %d, "
			"start "
			")",
			(int)syscall(SYS_gettid)
			);

	// load data from database as mysql
	timeval tStart;
	timeval tEnd;

	char *msg = NULL;
	char sql[1024] = {'\0'};
	char ids[64] = {'\0'};

	sqlite3_stmt **stmtMan = new sqlite3_stmt*[miMaxMemoryCopy];
	sqlite3_stmt **stmtLady = new sqlite3_stmt*[miMaxMemoryCopy];

	// update memory
	gettimeofday(&tStart, NULL);

	sprintf(sql,
			"SELECT `id`, `q_id`, `question_status`, `content`, `manid`, `manname`, `country`, `answer_id`, `answer`, `q_concat`, `answer_time`, UNIX_TIMESTAMP(`last_update`), `ip`, `siteid` FROM mq_man_answer WHERE UNIX_TIMESTAMP(`last_update`) > %lld"
			";",
			miLastUpdateMan
	);
	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"DBManager::SyncDataFromDataBase( "
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
				"DBManager::SyncDataFromDataBase( "
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
				sprintf(sql, "REPLACE INTO man("
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

//				for( int j = 0; j < iFields; j++ ) {
//					LogManager::GetLogManager()->Log(
//							LOG_STAT,
//							"DBManager::SyncDataFromDataBase( "
//							"tid : %d, "
//							"row[%d] : %s "
//							")",
//							(int)syscall(SYS_gettid),
//							j,
//							row[j]
//							);
//				}

				if( row[0] ) {
					int temp = atoi(row[0]);
					miLastManRecordId = temp;
				}

				for( int j = 0; j < miMaxMemoryCopy; j++ ) {
					// insert man
					InsertManFromDataBase(stmtMan[j], row, iFields);
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
			"SELECT `id`,`q_id`, `question_status`, `content`, `womanid`, `womanname`, `agent`, `answer_id`, `answer`, `q_concat`, `answer_time`, UNIX_TIMESTAMP(`last_update`), `siteid` FROM mq_woman_answer WHERE UNIX_TIMESTAMP(`last_update`) > %lld"
			";",
			miLastUpdateLady
	);
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncDataFromDataBase( "
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
						"DBManager::SyncDataFromDataBase( "
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
				sprintf(sql, "REPLACE INTO woman("
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
					InsertLadyFromDataBase(stmtLady[j], row, iFields);
				}
			}

			for( int i = 0; i < miMaxMemoryCopy; i++ ) {
				sqlite3_finalize(stmtLady[i]);
				ExecSQL( mdbs[i], "COMMIT;", NULL );
			}
		}
	}
	mDBSpool.ReleaseConnection(shIdt);

	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));

	if( stmtMan ) {
		delete stmtMan;
	}

	if( stmtLady ) {
		delete stmtLady;
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"DBManager::SyncDataFromDataBase( "
			"tid : %d, "
			"time : %ldus "
			"end "
			")",
			(int)syscall(SYS_gettid),
			usec
			);
}

void DBManager::Status() {
	sqlite3* db;
	int current;
	int highwater;

	for( int i = 0; i < miMaxMemoryCopy; i++ ) {
		sqlite3* db = mdbs[i];
		sqlite3_db_status(db, SQLITE_STATUS_MEMORY_USED, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_MEMORY_USED, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_MALLOC_SIZE, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_MALLOC_SIZE, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_MALLOC_COUNT, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_MALLOC_COUNT, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_PAGECACHE_USED, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_PAGECACHE_USED, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_SCRATCH_USED, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_SCRATCH_USED, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_SCRATCH_OVERFLOW, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_SCRATCH_OVERFLOW, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_SCRATCH_SIZE, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_SCRATCH_SIZE, current : %d, highwater : %d \n", i, current, highwater);

		sqlite3_db_status(db, SQLITE_STATUS_PARSER_STACK, &current, &highwater, true);
		printf("# db[%d] SQLITE_STATUS_PARSER_STACK, current : %d, highwater : %d \n", i, current, highwater);
	}

}

int DBManager::GetSyncDataTime() {
	return mSyncDataTime;
}

void DBManager::SetSyncDataTime(int second) {
	mSyncDataTime = second;
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
			"CREATE TABLE man("
//						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"id INT PRIMARY KEY,"
						"qid BIGINT,"
						"question_status INT,"
						"manid VARCHAR(12),"
						"aid INT,"
						"siteid INT"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table man ok \n");

	// 建男士表索引(manid, qid, aid)
	sprintf(sql,
			"CREATE INDEX manindex_manid_qid_aid "
			"ON man (manid, qid, aid)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建男士表索引(qid, aid)
	sprintf(sql,
			"CREATE INDEX manindex_qid_aid "
			"ON man (qid, aid)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table man index ok \n");

	// 建女士表
	sprintf(sql,
			"CREATE TABLE woman("
//						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"id INT PRIMARY KEY,"
						"qid BIGINT,"
						"question_status INT,"
						"womanid VARCHAR(50),"
						"aid INT,"
						"siteid INT"
						");"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table woman, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table woman ok \n");

	// 建女士表索引(qid, aid, question_status)
	sprintf(sql,
			"CREATE INDEX womanindex_qid_aid_question_status "
			"ON woman (qid, aid, question_status)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table woman index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}

	// 建女士表索引(LADYID, QID, AID)
	sprintf(sql,
			"CREATE INDEX womanindex_womanid_qid_aid "
			"ON woman (womanid, qid, aid)"
			";"
	);

	ExecSQL( db, sql, &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table woman index, msg: %s \n", msg);
		sqlite3_free(msg);
		msg = NULL;
		return false;
	}
	printf("# Create table woman index ok \n");

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

	sprintf(sql, "INSERT INTO man(`id`, `qid`, `question_status`, `manid`, `aid`, `siteid`) VALUES(?, ?, 1, ?, ?, 1)");
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmtMan, 0);

	sprintf(sql, "INSERT INTO woman(`id`, `qid`, `question_status`, `womanid`, `aid`, `siteid`) VALUES(?, ?, 1, ?, ?, 1)");
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmtLady, 0);

	// 插入表
	gettimeofday(&tStart, NULL);
	for( int i = 0; i < 40000; i++ ) {

		sprintf(id, "%d", i);
		sprintf(idlady, "%d", i);

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
			sqlite3_bind_int(stmtMan, 1, miLastManRecordId);
			sqlite3_bind_int(stmtMan, 2, j);
			sqlite3_bind_text(stmtMan, 3, id, strlen(id), NULL);
			sqlite3_bind_int(stmtMan, 4, r);
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

bool DBManager::InsertManFromDataBase(sqlite3_stmt *stmtMan, MYSQL_ROW &row, int iFields) {
	sqlite3_reset(stmtMan);

	if( iFields == 14 ) {
		// id
		if( row[0] ) {
			sqlite3_bind_int(stmtMan, 1, atoi(row[0]));
		}
		// q_id -> qid
		if( row[1] ) {
			string qid = row[1];
			qid = qid.substr(2, qid.length() - 2);
			sqlite3_bind_int(stmtMan, 2, atoll(qid.c_str()));
		}
		// question_status
		if( row[2] ) {
			sqlite3_bind_int(stmtMan, 3, atoi(row[2]));
		}
//		// content
//		if( row[3] ) {
//			sqlite3_bind_text(stmtMan, 4, row[3], strlen(row[3]), NULL);
//		}
		// manid
		if( row[4] ) {
			sqlite3_bind_text(stmtMan, 4, row[4], strlen(row[4]), NULL);
		}
//		// manname
//		if( row[5] ) {
//			sqlite3_bind_text(stmtMan, 6, row[5], strlen(row[5]), NULL);
//		}
//		// country
//		if( row[6] ) {
//			sqlite3_bind_text(stmtMan, 7, row[6], strlen(row[6]), NULL);
//		}
//		// answer_id
//		if( row[7] ) {
//			sqlite3_bind_int(stmtMan, 8, atoi(row[7]));
//		}
//		// answer
//		if( row[8] ) {
//			sqlite3_bind_text(stmtMan, 9, row[8], strlen(row[8]), NULL);
//		}
		// q_concat -> aid
		if( row[9] ) {
			string straid = row[9];
			std::string::size_type pos = straid.find('-', 0);
			if( pos != std::string::npos && pos != (straid.length() - 1) ) {
				straid = straid.substr(pos + 1, straid.length() - (pos + 1));
				sqlite3_bind_int(stmtMan, 5, atoi(straid.c_str()));
			}
		}
//		// answer_time
//		if( row[10] ) {
//			sqlite3_bind_text(stmtMan, 11, row[10], strlen(row[10]), NULL);
//		}
		// last_update
		if( row[11] ) {
			int timestamp = atoll(row[11]);
			if( miLastUpdateMan < timestamp ) {
				miLastUpdateMan = timestamp;
			}
//			sqlite3_bind_text(stmtMan, 12, row[11], strlen(row[11]), NULL);
		}
//		// ip
//		if( row[12] ) {
//			sqlite3_bind_text(stmtMan, 13, row[12], strlen(row[12]), NULL);
//		}
		// siteid
		if( row[13] ) {
			sqlite3_bind_int(stmtMan, 6, atoi(row[13]));
		}

		int code = sqlite3_step(stmtMan);

	} else {
		return false;
	}

	return true;
}

bool DBManager::InsertLadyFromDataBase(sqlite3_stmt *stmtLady, MYSQL_ROW &row, int iFields) {
	sqlite3_reset(stmtLady);

	if( iFields == 13 ) {
		// id
		if( row[0] ) {
			sqlite3_bind_int(stmtLady, 1, atoi(row[0]));
		}
		// q_id -> qid
		if( row[1] ) {
			string qid = row[1];
			qid = qid.substr(2, qid.length() - 2);
			sqlite3_bind_int(stmtLady, 2, atoll(qid.c_str()));
		}
		// question_status
		if( row[2] ) {
			sqlite3_bind_int(stmtLady, 3, atoi(row[2]));
		}
//		// content
//		if( row[3] ) {
//			sqlite3_bind_text(stmtLady, 4, row[3], strlen(row[3]), NULL);
//		}
		// womanid
		if( row[4] ) {
			sqlite3_bind_text(stmtLady, 4, row[4], strlen(row[4]), NULL);
		}
//		// womanname
//		if( row[5] ) {
//			sqlite3_bind_text(stmtLady, 6, row[5], strlen(row[5]), NULL);
//		}
//		// agent
//		if( row[6] ) {
//			sqlite3_bind_text(stmtLady, 7, row[6], strlen(row[6]), NULL);
//		}
//		// answer_id
//		if( row[7] ) {
//			sqlite3_bind_int(stmtLady, 8, atoi(row[7]));
//		}
//		// answer
//		if( row[8] ) {
//			sqlite3_bind_text(stmtLady, 9, row[8], strlen(row[8]), NULL);
//		}
		// q_concat -> aid
		if( row[9] ) {
			string straid = row[9];
			std::string::size_type pos = straid.find('-', 0);
			if( pos != std::string::npos && pos != (straid.length() - 1) ) {
				straid = straid.substr(pos + 1, straid.length() - (pos + 1));
				sqlite3_bind_int(stmtLady, 5, atoi(straid.c_str()));
			}
		}
//		// answer_time
//		if( row[10] ) {
//			sqlite3_bind_text(stmtLady, 11, row[10], strlen(row[10]), NULL);
//		}
		// last_update
		if( row[11] ) {
			int timestamp = atoll(row[11]);
			if( miLastUpdateLady < timestamp ) {
				miLastUpdateLady = timestamp;
			}
//			sqlite3_bind_text(stmtLady, 12, row[11], strlen(row[11]), NULL);
		}
		// siteid
		if( row[12] ) {
			sqlite3_bind_int(stmtLady, 6, atoi(row[12]));
		}

		int code = sqlite3_step(stmtLady);
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
	mSyncMutex.lock();
	if( mbSyncForce ) {
		mbSyncForce = false;
		mSyncMutex.unlock();

		SyncDataFromDataBase();
		i = 0;
	} else {
		mSyncMutex.unlock();

		if( i > GetSyncDataTime() ) {
			SyncDataFromDataBase();
			i = 0;
		}
	}
	i++;
	sleep(1);
}
