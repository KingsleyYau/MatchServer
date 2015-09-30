//============================================================================
// Name        : MemoryDB.cpp
// Author      : Max.Chiu
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>

using namespace std;
#include <string.h>

#include <sqlite3.h>

#include "KThread.h"

//char sql[4096] = {'\0'};
//char id[65] = {'\0'};
//char qid[65] = {'\0'};
//char aid[65] = {'\0'};

char *msg = NULL;			// 错误提示
int iRow = 0, iColumn = 0; 	// 查询结果集的行数、列数
char **result; 				// 二维数组存放结果

int iMemory = 1;	// sqlite句柄数目,即内存数据库数目
bool bShow = true;	// 是否显示结果
int iThread = 1;	// 线程数目
int iTask = 1;		// 总查询次数
int iCur = 0;		// 已经查询次数

long iMin = -1;		// 最小处理时间
long iMax = -1;		// 最大处理时间
long iTol = 0;		// 总处理时间
long iAvg = 0;		// 平均处理时间

KMutex gMutex;

// 初始化数据
bool CreateTableAndData(sqlite3 *db);
bool Query(sqlite3 *db, char* sql, bool show);
bool Parse(int argc, char *argv[]);

/* handle thread */
class HandleRunnable : public KRunnable {
public:
	HandleRunnable(sqlite3 *db) {
		mdb = db;
	}
	virtual ~HandleRunnable() {
	}
protected:
	void onRun() {
//		int ret = sqlite3_open(":memory:", &mdb);
		sprintf(sql, "SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = 'man-10000';");
//
//		if( !CreateTableAndData(mdb) ) {
//			return;
//		}

		int times = iTask / iThread;
		for( int i = 0; i < times; i++ ) {
			Query(mdb, sql, bShow);
		}
	}

	sqlite3 *mdb;
	char sql[4096] = {'\0'};
};


int main(int argc, char *argv[]) {
	puts("# Hello World!!!");
	srand((int)time(0));
	sqlite3_config(SQLITE_CONFIG_MEMSTATUS, false);

	Parse(argc, argv);

//	sqlite3 *db;
//	int ret = sqlite3_open(":memory:", &db);
//	if ( ret != SQLITE_OK ) {
//		fprintf(stderr, "# Could not open database: %s \n", sqlite3_errmsg(db));
//	    exit(1);
//	}
//	printf("# Successfully connected to database \n");
//
//	if( !CreateTableAndData(db) ) {
//		return EXIT_FAILURE;
//	}

	// 查询数据
//	do {
		char sql[4096] = {'\0'};
		memset(sql, '\0', sizeof(sql));
//		printf("# Please input sql : \n");
//		gets(sql);
		sprintf(sql, "SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = 'man-10000';");
		if( strlen(sql) > 0 ) {
			iCur = 0;
			iTol = 0;
			iAvg = 0;
			iMax = -1;
			iMin = -1;
			KThread** array = new KThread*[iThread];
			sqlite3** db = new sqlite3*[iThread];

			for( int i = 0; i < iMemory; i++ ) {
				int ret = sqlite3_open(":memory:", &db[i]);
				if( ret != SQLITE_OK || !CreateTableAndData(db[i]) ) {
					return EXIT_FAILURE;
				}
			}

			printf("# Start quering...  \n");
			for( int i = 0; i < iThread; i++ ) {
				KThread *thread = new KThread(new HandleRunnable(db[ i % iMemory ]));
				thread->start();
				array[i] = thread;
			}

			timeval tStart;
			timeval tEnd;
			gettimeofday(&tStart, NULL);

			while(true) {
				usleep(10);
				if( iCur >= iTask ) {
					gettimeofday(&tEnd, NULL);
					long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
					iAvg = iTol / iTask;

					printf("# 单次查询最小用时 : %ldus, \n"
							"# 单次查询最大用时 : %ldus, \n"
							"# 单次查询平均用时 : %ldus, \n"
//							"# 每次查询用时总和 : %ldus, \n"
							"# %d次查询 : %ldus, \n"
							"# 平均%ld次查询每秒 \n",
							iMin,
							iMax,
							iAvg,
//							iTol,
							iTask,
							usec,
							(long)iTask / (usec / 1000 / 1000 )
							);
					break;
				}
			}

			for( int i = 0; i < iThread; i++ ) {
				delete array[i];
				array[i] = NULL;
			}
			delete array;
			array = NULL;

//			if( !Query(db, sql, bShow) ) {
//				printf("# Query fail \n");
//			} else {
//				printf("# Query finish \n");
//			}
		}

//	} while( true );

	return EXIT_SUCCESS;
}

// 初始化数据
bool CreateTableAndData(sqlite3 *db) {
	char sql[4096] = {'\0'};
	char id[65] = {'\0'};
	char qid[65] = {'\0'};
	char aid[65] = {'\0'};

	timeval tStart;
	timeval tEnd;

	// 建男士表
	sprintf(sql,
			"CREATE TABLE MAN("
						"ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						"MANID VARCHAR(64),"
						"QID INT,"
						"AID INT"
						");"
	);
	sqlite3_exec( db , sql , NULL , NULL , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man, msg: %s \n", msg);
		return false;
	}
	printf("# Create table MAN ok \n");

	// 建男士表索引(MANID, QID, AID)
	sprintf(sql,
			"CREATE INDEX MANINDEX_MANID_QID_AID "
			"ON MAN (MANID, QID, AID)"
			";"
	);
	sqlite3_exec( db , sql , NULL , NULL , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man index, msg: %s \n", msg);
		return false;
	}

	// 建男士表索引(QID, AID)
	sprintf(sql,
			"CREATE INDEX MANINDEX_QID_AID "
			"ON MAN (QID, AID)"
			";"
		);
	sqlite3_exec( db , sql , NULL , NULL , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table man index, msg: %s \n", msg);
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
	sqlite3_exec( db , sql , NULL , NULL , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table lady, msg: %s \n", msg);
		return false;
	}
	printf("# Create table LADY ok \n");

	// 建女士表索引(QID, AID)
	sprintf(sql,
			"CREATE INDEX LADYINDEX_QID_AID "
			"ON LADY (QID, AID)"
			";"
	);
	sqlite3_exec( db , sql , NULL , NULL , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table lady index, msg: %s \n", msg);
		return false;
	}

	// 建女士表索引(LADYID, QID, AID)
	sprintf(sql,
			"CREATE INDEX LADYINDEX_LADYID_QID_AID "
			"ON LADY (LADYID, QID, AID)"
			";"
	);
	sqlite3_exec( db , sql , NULL , NULL , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not create table lady index, msg: %s \n", msg);
		return false;
	}
	printf("# Create table LADY index ok \n");

	sqlite3_exec( db, "BEGIN TRANSACTION;", 0, 0, &msg );
	// 插入男士表
	gettimeofday(&tStart, NULL);
	for( int i = 0; i < 40000; i++ ) {

		sprintf(id, "man-%d", i);

		for( int j = 0; j < 30; j++ ) {
			sprintf(qid, "%d", j);
			int r = rand() % 30;
			sprintf(aid, "%d", r);

			// insert man
			sprintf(sql, "INSERT INTO MAN(`MANID`, `QID`, `AID`) VALUES('%s', %d, %d);",
					id,
					j,
					r
					);
//			printf("# sql : %s \n", sql);
			sqlite3_exec( db , sql , 0 , 0 , &msg );
			if( msg != NULL ) {
				fprintf(stderr, "# Could not insert table man, msg : %s \n", msg);
				return false;
			}

			// insert lady
			r = rand() % 30;
			sprintf(aid, "%d", r);
			sprintf(sql, "INSERT INTO LADY(`LADYID`, `QID`, `AID`) VALUES('%s', %d, %d);",
					id,
					j,
					r
					);
//			printf("# sql : %s \n", sql);
			sqlite3_exec( db , sql , 0 , 0 , &msg );

			if( msg != NULL ) {
				fprintf(stderr, "# Could not insert table lady, msg : %s \n", msg);
				return false;
			}
		}
	}
	sqlite3_exec( db, "COMMIT TRANSACTION;", 0, 0, &msg );
	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
	printf("# Insert table ok time: %ldus \n", usec);

	return true;
}

bool Query(sqlite3 *db, char* sql, bool show) {
//	printf("# Query() \n");
//	printf("# sql : %s \n", sql);
	timeval tStart;
	timeval tEnd;

	bool bFlag = true;
	gettimeofday(&tStart, NULL);
	sqlite3_get_table( db , sql , &result , &iRow , &iColumn , &msg );
	if( msg != NULL ) {
		fprintf(stderr, "# Could not select table, msg : %s \n", msg);
		bFlag = false;
	}

	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));

	gMutex.lock();
	if( usec > iMax || iMax == -1 ) {
		iMax = usec;
	}
	if( usec < iMin || iMin == -1 ) {
		iMin = usec;
	}
	iTol += usec;
	iCur++;
	gMutex.unlock();

//	printf("# row : %d column : %d, time: %ldus, iCur : %d \n" , iRow , iColumn, usec, iCur);
	if( bFlag && show ) {
		printf("# The result of querying is : \n");
		for( int i = 0 ; i < ( iRow + 1 ); i++ ) {
			for( int j = 0; j < iColumn; j++  ) {
				printf("%12s |", result[i * iColumn + j]);
			}
			printf("\n");
		}
	}

	// 释放掉result 的内存空间
	sqlite3_free_table(result);

	return bFlag;
}

bool Parse(int argc, char *argv[]) {
	string key, value;
	for( int i = 1; (i + 1) < argc; i+=2 ) {
//		printf("argv[%d] : %s \n", i, argv[i]);
		key = argv[i];
		value = argv[i+1];

//		printf("key : %s, value : %s \n", key.c_str(), value.c_str());
		if( key.compare("-t") == 0 ) {
			iThread = atoi(value.c_str());
		} else if( key.compare("-n") == 0 ) {
			iTask = atoi(value.c_str());
		} else if(  key.compare("-m") == 0 ) {
			iMemory = atoi(value.c_str());
		} else if( key.compare("-b") == 0 ) {
			bShow = atoi(value.c_str());
		}
	}

	printf("# iThread : %d, iTask : %d, iMemory : %d, bShow : %s \n", iThread, iTask, iMemory, bShow?"true":"false");

	return true;
}
