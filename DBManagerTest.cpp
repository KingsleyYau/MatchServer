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
		sprintf(sql, "SELECT * FROM MAN WHERE MANID = 'man-100';"); // 78us

		timeval tStart;
		timeval tEnd;
		timeval tStart2;
		timeval tEnd2;

		bool bResult = false;
		char** result;
		int iRow;
		int iColumn;

		char** result2;
		int iRow2;
		int iColumn2;
		int qid = 0;
		int aid = 0;

		for( int i = 0; mpDBManagerTest->miCur < mpDBManagerTest->miMaxQuery; i++ ) {
			gettimeofday(&tStart, NULL);

			bResult = mpDBManagerTest->mDBManager.Query(sql, &result, &iRow, &iColumn);
//			printf("iRow %d, iColumn : %d, result : %p \n", iRow, iColumn, result);
			if( bResult && result ) {
				for( int j = 0; j < (iRow + 1); j++ ) {
//					for( int k = 0; k < iColumn; k++ ) {
//						printf("%8s |", result[j * iColumn + k]);
//					}
//					printf("\n");
					gettimeofday(&tStart2, NULL);
					qid = atoi(result[j * iColumn + 2]);
					aid = atoi(result[j * iColumn + 3]);
					sprintf(sql2, "SELECT COUNT(DISTINCT ID) FROM LADY WHERE QID = %d AND AID = %d;", qid, aid);
//					printf("sql2 : %s \n", sql2);
					bResult = mpDBManagerTest->mDBManager.Query(sql2, &result2, &iRow2, &iColumn2);
//					printf("iRow2 %d, iColumn2 : %d, result2 : %p \n", iRow2, iColumn2, result2);
					if( bResult && result2 ) {
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

					mpDBManagerTest->mMutex.lock();
					mpDBManagerTest->miQuery += usec2;
					mpDBManagerTest->mMutex.unlock();

					gettimeofday(&tStart2, NULL);
					usleep(usec2);
					gettimeofday(&tEnd2, NULL);

					long usec3 = (1000 * 1000 * tEnd2.tv_sec + tEnd2.tv_usec - (1000 * 1000 * tStart2.tv_sec + tStart2.tv_usec));
					mpDBManagerTest->mMutex.lock();
					mpDBManagerTest->miSleep += usec3;
					mpDBManagerTest->mMutex.unlock();
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

	miMaxQuery = iMaxQuery;
	mDBManager.Init(iMaxMemoryCopy, true);
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
		usleep(10);
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
					"# 平均每秒并发查询%ld次 , \n"
					"# miQuery : %ldus, \n"
					"# miSleep : %ldus \n ",
					miMin,
					miMax,
					miAvg,
//					iTol,
					miMaxQuery,
					usec,
					(long)miMaxQuery / avg,
					miQuery,
					miSleep
					);
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
