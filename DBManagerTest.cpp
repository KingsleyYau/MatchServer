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
		sprintf(sql, "SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = 'man-10000';");

		timeval tStart;
		timeval tEnd;

		bool bResult = false;
		char** result;
		int iRow;
		int iColumn;

		for( int i = 0; mpDBManagerTest->miCur < mpDBManagerTest->miMaxQuery; i++ ) {
			gettimeofday(&tStart, NULL);

			bResult = mpDBManagerTest->mDBManager.Query(sql, result, &iRow, &iColumn);
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
			miAvg = miTol / iMaxQuery;

			printf("# 单次查询最小用时 : %ldus, \n"
					"# 单次查询最大用时 : %ldus, \n"
					"# 单次查询平均用时 : %ldus, \n"
//					"# 每次查询用时总和 : %ldus, \n"
					"# %d次查询 : %ldus, \n"
					"# 平均每秒并发查询%ld次 \n",
					miMin,
					miMax,
					miAvg,
//					iTol,
					miMaxQuery,
					usec,
					(long)miMaxQuery / (usec / 1000 / 1000 )
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
