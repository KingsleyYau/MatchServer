/*
 * DBManagerTest.cpp
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#include "DBManagerTest.h"

class TestRunnable : public KRunnable {
public:
	TestRunnable(DBManagerTest* pDBManagerTest) {
		mpDBManagerTest = pDBManagerTest;
	}
	virtual ~TestRunnable() {
	}
protected:
	void onRun() {
		char sql[2048] = {'\0'};
		char sql2[2048] = {'\0'};
//		sprintf(sql, "SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = 'man-10000';");
		sprintf(sql, "SELECT qid, aid FROM man WHERE manid = 'CM100';"); // 78us

		timeval tStart;
		timeval tEnd;
		timeval tStart1;
		timeval tEnd1;
		timeval tStart2;
		timeval tEnd2;

		bool bResult = false;
		char** result;
		int iRow;
		int iColumn;

		char** result2;
		int iRow2;
		int iColumn2;
		long long qid = 0;
		int aid = 0;

		for( int i = 0; mpDBManagerTest->miCur < mpDBManagerTest->miMaxQuery; i++ ) {
			gettimeofday(&tStart, NULL);
//			gettimeofday(&tStart1, NULL);
			bResult = mpDBManagerTest->mDBManager.Query(sql, &result, &iRow, &iColumn);
//			gettimeofday(&tEnd1, NULL);
//			long usec1 = (1000 * 1000 * tEnd1.tv_sec + tEnd1.tv_usec - (1000 * 1000 * tStart1.tv_sec + tStart1.tv_usec));
//			usleep(usec1);
//			printf("iRow %d, iColumn : %d, result : %p \n", iRow, iColumn, result);
			if( bResult && result && iRow > 0 ) {
				for( int j = 1; j < (iRow + 1); j++ ) {
//					for( int k = 0; k < iColumn; k++ ) {
//						printf("%8s |", result[j * iColumn + k]);
//					}
//					printf("\n");
					gettimeofday(&tStart2, NULL);
//					qid = atoll(result[j * iColumn + 1]);
					qid = atoll(result[j * iColumn]);
					aid = atoi(result[j * iColumn + 1]);
//					sprintf(sql2, "SELECT * FROM woman WHERE qid = %lld AND aid = %d AND question_status=1 AND siteid = 1;", qid, aid);
					sprintf(sql2, "SELECT womanid FROM woman WHERE qid = %lld AND aid = %d;", qid, aid);
//					printf("sql2 : %s \n", sql2);
					bResult = mpDBManagerTest->mDBManager.Query(sql2, &result2, &iRow2, &iColumn2);
//					printf("iRow2 %d, iColumn2 : %d, result2 : %p \n", iRow2, iColumn2, result2);
					if( bResult && result2 && iRow2 > 0 ) {
//						for( int j2 = 0; j2 < (iRow2 + 1); j2++ ) {
//							for( int k = 0; k < iColumn2; k++ ) {
//								printf("%8s |", result2[j2 * iColumn2 + k]);
//							}
//							printf("\n");
//						}
					}
					mpDBManagerTest->mDBManager.FinishQuery(result2);

					gettimeofday(&tEnd2, NULL);
					long usec2 = (1000 * 1000 * tEnd2.tv_sec + tEnd2.tv_usec - (1000 * 1000 * tStart2.tv_sec + tStart2.tv_usec));

//					mpDBManagerTest->mMutex.lock();
//					mpDBManagerTest->miQuery += usec2;
//					mpDBManagerTest->mMutex.unlock();

					gettimeofday(&tStart2, NULL);
					usleep(usec2);
					gettimeofday(&tEnd2, NULL);

//					long usec3 = (1000 * 1000 * tEnd2.tv_sec + tEnd2.tv_usec - (1000 * 1000 * tStart2.tv_sec + tStart2.tv_usec));
//					mpDBManagerTest->mMutex.lock();
//					mpDBManagerTest->miSleep += usec3;
//					mpDBManagerTest->mMutex.unlock();
				}
			}
			mpDBManagerTest->mDBManager.FinishQuery(result);

			gettimeofday(&tEnd, NULL);
			long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));

			mpDBManagerTest->mMutex.lock();
			if( usec > mpDBManagerTest->miMax || mpDBManagerTest->miMax == -1 ) {
				mpDBManagerTest->miMax = usec;
			}
			if( usec < mpDBManagerTest->miMin || mpDBManagerTest->miMin == -1 ) {
				mpDBManagerTest->miMin = usec;
			}
			mpDBManagerTest->miTol += usec;
			mpDBManagerTest->miCur++;
			mpDBManagerTest->mMutex.unlock();

		}
	}

	DBManagerTest* mpDBManagerTest;
};

DBManagerTest::DBManagerTest() {
	// TODO Auto-generated constructor stub
	miQuery = 0;
	miSleep = 0;
	miCur = 0;
	miTol = 0;
	miAvg = 0;
	miMax = -1;
	miMin = -1;
}

DBManagerTest::~DBManagerTest() {
	// TODO Auto-generated destructor stub
}

void DBManagerTest::StartTest(int iMaxThread, int iMaxMemoryCopy, int iMaxQuery) {
	timeval tStart;
	timeval tEnd;
	gettimeofday(&tStart, NULL);

	miMaxQuery = iMaxQuery;
	mDBManager.Init(iMaxMemoryCopy, true);
//	mDBManager.InitSyncDataBase(4, "127.0.0.1", 3306, "qpidnetwork", "root", "123456");

	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
	printf("# 同步数据用时 : %ldus\n", usec);

	KThread** threads = new KThread*[iMaxThread];
	TestRunnable** runnable = new TestRunnable*[iMaxThread];

	for( int i = 0; i < iMaxThread; i++ ) {
		runnable[i] = new TestRunnable(this);
		KThread *thread = new KThread(runnable[i]);
		thread->start();
		threads[i] = thread;
	}

	gettimeofday(&tStart, NULL);

	while(true) {
		usleep(1000);
		if( miCur >= iMaxQuery ) {
			gettimeofday(&tEnd, NULL);
			long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
			long avg = usec / 1000 / 1000;
			avg = (avg == 0)?1:avg;
			miAvg = miTol / iMaxQuery;

			printf("# 单次查询最小用时 : %ldus, \n"
					"# 单次查询最大用时 : %ldus, \n"
					"# 单次查询平均用时 : %ldus, \n"
//					"# 每次查询用时总和 : %ldus, \n"
					"# %d次查询 : %ldus, \n"
					"# 平均每秒并发查询%ld次  \n",
//					"# miQuery : %ldus, \n"
//					"# miSleep : %ldus \n ",
					miMin,
					miMax,
					miAvg,
//					iTol,
					miMaxQuery,
					usec,
					(long)miMaxQuery / avg
//					miQuery,
//					miSleep
					);

			mDBManager.Status();
			break;
		}
	}

	for( int i = 0; i < iMaxThread; i++ ) {
		threads[i]->stop();
		delete threads[i];
		delete runnable[i];
		threads[i] = NULL;
		runnable[i] = NULL;
	}
	delete threads;
	threads = NULL;
}

void DBManagerTest::TestSql(string sql) {
	mDBManager.Init(1, true);
	bool bResult = false;
	char** result;
	int iRow;
	int iColumn;
	bResult = mDBManager.Query((char*)sql.c_str(), &result, &iRow, &iColumn);

	if( bResult && result && iRow > 0 ) {
		for( int j = 1; j < (iRow + 1); j++ ) {
			for( int k = 0; k < iColumn; k++ ) {
				printf("%8s |", result[j * iColumn + k]);
			}
			printf("\n");
		}
	}

}
