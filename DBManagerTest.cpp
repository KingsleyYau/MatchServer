/*
 * DBManagerTest.cpp
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#include "DBManagerTest.h"

#include <map>
using namespace std;

class TestRunnable : public KRunnable {
public:
	TestRunnable(DBManagerTest* pDBManagerTest, int index) {
		mpDBManagerTest = pDBManagerTest;
		mIndex = index;
	}
	virtual ~TestRunnable() {
	}
protected:
	void onRun() {

		timeval tStart;
		timeval tEnd;

		for( int i = 0; mpDBManagerTest->miCur < mpDBManagerTest->miMaxQuery; i++ ) {
			gettimeofday(&tStart, NULL);

//			mpDBManagerTest->Test1(mIndex);
//			mpDBManagerTest->Test2(mIndex);
			mpDBManagerTest->Test3(mIndex);
//			mpDBManagerTest->Test4(mIndex);

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
	int mIndex;
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
	printf("# 同步数据用时 : %lds\n", usec / 1000 / 1000);

	KThread** threads = new KThread*[iMaxThread];
	TestRunnable** runnable = new TestRunnable*[iMaxThread];

	for( int i = 0; i < iMaxThread; i++ ) {
		runnable[i] = new TestRunnable(this, i);
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

//			mDBManager.Status();
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

void DBManagerTest::Test1(int index) {
	char sql[1024] = {'\0'};
	bool bResult = false;
	char** result;
	int iRow;
	int iColumn;

	char** result2;
	int iRow2;
	int iColumn2;

	timeval tStart;
	timeval tEnd;

	sprintf(sql, "SELECT qid, aid FROM man WHERE manid = 'CM100';"); // 78us

	gettimeofday(&tStart, NULL);
	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn);
	if( bResult && result && iRow > 0 ) {
		for( int j = 1; j < (iRow + 1); j++ ) {
			gettimeofday(&tStart, NULL);
			sprintf(sql, "SELECT womanid FROM woman WHERE qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;", result[j * iColumn], result[j * iColumn + 1]);
			bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, index);
//			printf("iRow2 %d, iColumn2 : %d, result2 : %p \n", iRow2, iColumn2, result2);
			if( bResult && result2 && iRow2 > 0 ) {
			}
			mDBManager.FinishQuery(result2);

			gettimeofday(&tEnd, NULL);
			long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));

			gettimeofday(&tStart, NULL);
			usleep(usec);
			gettimeofday(&tEnd, NULL);

		}
	}
	mDBManager.FinishQuery(result);
}

void DBManagerTest::Test2(int index) {
	// 执行查询
	char sql[1024] = {'\0'};
	string womanIds = "";

	sprintf(sql, "SELECT qid, aid FROM man WHERE manid = 'CM100';");

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	timeval tStart;
	timeval tEnd;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn);

	if( bResult && result && iRow > 0 ) {
		// 随机起始查询问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;
		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			gettimeofday(&tStart, NULL);
			if( !bEnougthLady ) {
				// query more lady
				sprintf(sql, "SELECT womanid FROM woman WHERE qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;",
						result[i * iColumn],
						result[i * iColumn + 1]
						);

				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, index);
				if( bResult && result2 && iRow2 > 0 ) {
					int iLadyIndex = 1;
					int iLadyCount = 0;

					if( iRow2 + womanidMap.size() >= 30 ) {
						bEnougthLady = true;
						iLadyCount = 30 - womanidMap.size();
						iLadyIndex = (rand() % (iRow2 -iLadyCount)) + 1;
					} else {
						iLadyCount = iRow2;
					}

					for( int j = iLadyIndex, k = 0; (j < iRow2 + 1) && (k < iLadyCount); k++, j++ ) {
						// find womanid
						womanid = result2[j * iColumn2];
						womanidMap.insert(map<string, int>::value_type(womanid, 1));
					}
				}
				mDBManager.FinishQuery(result2);
			} else {
				for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
					char** result3 = NULL;
					int iRow3;
					int iColumn3;

					sprintf(sql, "SELECT count(*) FROM woman WHERE womanid = '%s' AND qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;",
							itr->first.c_str(),
							result[i * iColumn],
							result[i * iColumn + 1]
							);

					bResult = mDBManager.Query(sql, &result3, &iRow3, &iColumn3, index);
					if( bResult && result3 && iRow3 > 0 ) {
						if( strcmp(result3[1 * iColumn3], "0") != 0 ) {
							itr->second++;
						}
					}
					mDBManager.FinishQuery(result3);
				}
			}

			i++;
			i = ((i - 1) % iRow) + 1;
			gettimeofday(&tEnd, NULL);
			long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
			usleep(usec);
		}
	}
	mDBManager.FinishQuery(result);
}
void DBManagerTest::Test3(int index) {
	// 执行查询
	char sql[1024] = {'\0'};
	string womanIds = "";

	sprintf(sql, "SELECT qid, aid FROM man WHERE manid = 'CM100';");

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	timeval tStart;
	timeval tEnd;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn);
	if( bResult && result && iRow > 0 ) {
		// 随机起始查询问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;
		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			gettimeofday(&tStart, NULL);
			if( !bEnougthLady ) {
				// query more lady
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				int iNum = 0;

				sprintf(sql, "SELECT count(*) FROM woman WHERE qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;",
										result[i * iColumn],
										result[i * iColumn + 1]
										);
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, index);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				int iLadyIndex = 1;
				int iLadyCount = 0;
				if( iNum + womanidMap.size() >= 30 ) {
					bEnougthLady = true;
					iLadyCount = 30 - womanidMap.size();
					iLadyIndex = (rand() % (iNum -iLadyCount)) + 1;
				} else {
					iLadyCount = iNum;
				}

				sprintf(sql, "SELECT womanid FROM woman WHERE qid = %s AND aid = %s AND siteid = 1 AND question_status = 1 LIMIT %d OFFSET %d;",
						result[i * iColumn],
						result[i * iColumn + 1],
						iLadyCount,
						iLadyIndex
						);

				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, index);
				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// find womanid
						womanid = result2[j * iColumn2];
						womanidMap.insert(map<string, int>::value_type(womanid, 1));
					}
				}
				mDBManager.FinishQuery(result2);
			} else {
				for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
					char** result3 = NULL;
					int iRow3;
					int iColumn3;

					sprintf(sql, "SELECT count(*) FROM woman WHERE womanid = '%s' AND qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;",
							itr->first.c_str(),
							result[i * iColumn],
							result[i * iColumn + 1]
							);

					bResult = mDBManager.Query(sql, &result3, &iRow3, &iColumn3, index);
					if( bResult && result3 && iRow3 > 0 ) {
						if( strcmp(result3[1 * iColumn3], "0") != 0 ) {
							itr->second++;
						}
					}
					mDBManager.FinishQuery(result3);
				}
			}

			i++;
			i = ((i - 1) % iRow) + 1;

			gettimeofday(&tEnd, NULL);
			long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
			usleep(usec);
		}
	}
	mDBManager.FinishQuery(result);
}
void DBManagerTest::Test4(int index) {
	// 执行查询
	char sql[1024] = {'\0'};
	string womanIds = "";

	sprintf(sql, "SELECT qid, aid FROM man WHERE manid = 'CM100';");

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	timeval tStart;
	timeval tEnd;

	int iIndex = 0;
	string sWomanIds[30];
	int iSameAnswerQuestion[30];
	string womanid;

//	gettimeofday(&tStart, NULL);

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, index);
	if( bResult && result && iRow > 0 ) {
		// 随机起始查询问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;
		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			if( !bEnougthLady ) {
				// query more lady
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				int iNum = 0;

				sprintf(sql, "SELECT count(*) FROM woman WHERE qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;",
										result[i * iColumn],
										result[i * iColumn + 1]
										);
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, index);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				int iLadyIndex = 1;
				int iLadyCount = 0;
				if( iNum + iIndex >= 30 ) {
					bEnougthLady = true;
					iLadyCount = 30 - iIndex;
					iLadyIndex = (rand() % (iNum -iLadyCount)) + 1;
				} else {
					iLadyCount = iNum;
				}

				sprintf(sql, "SELECT womanid FROM woman WHERE qid = %s AND aid = %s AND siteid = 1 AND question_status = 1 LIMIT %d OFFSET %d;",
						result[i * iColumn],
						result[i * iColumn + 1],
						iLadyCount,
						iLadyIndex
						);

				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, index);
				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// find womanid
//						womanid = result2[j * iColumn2];
						sWomanIds[iIndex] = result2[j * iColumn2];
						iSameAnswerQuestion[iIndex] = 1;
						iIndex++;
					}
				}
				mDBManager.FinishQuery(result2);
			} else {
				for( int j = 0; j < iIndex; j++ ) {
					char** result3 = NULL;
					int iRow3;
					int iColumn3;

					sprintf(sql, "SELECT count(*) FROM woman WHERE womanid = '%s' AND qid = %s AND aid = %s AND siteid = 1 AND question_status = 1;",
							sWomanIds[iIndex].c_str(),
							result[i * iColumn],
							result[i * iColumn + 1]
							);

					bResult = mDBManager.Query(sql, &result3, &iRow3, &iColumn3, index);
					if( bResult && result3 && iRow3 > 0 ) {
						if( strcmp(result3[1 * iColumn3], "0") != 0 ) {
							iSameAnswerQuestion[j]++;
						}
					}
					mDBManager.FinishQuery(result3);
				}
			}

			i++;
			i = ((i - 1) % iRow) + 1;
		}
	}
	mDBManager.FinishQuery(result);

//	usleep(5000);
//	gettimeofday(&tEnd, NULL);
//	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
//	usleep(usec);
}
