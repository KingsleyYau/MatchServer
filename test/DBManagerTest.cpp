/*
 * DBManagerTest.cpp
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#include "DBManagerTest.h"
#include "TimeProc.hpp"
#include "json/json.h"

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
//			mpDBManagerTest->Test3(mIndex);
//			mpDBManagerTest->Test4(mIndex);

//			mpDBManagerTest->TestQuerySameAnswerLadyList(mIndex);				// 25 times/second
//			mpDBManagerTest->TestQueryTheSameQuestionLadyList(mIndex); 			// 180 times/second
//			mpDBManagerTest->TestQueryAnySameQuestionLadyList(mIndex);			// 135 time/second
			mpDBManagerTest->TestQueryAnySameQuestionOnlineLadyList(mIndex);	// 15 times/second

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

	Reload();
	miMaxQuery = iMaxQuery;
	mDBManager.Init(
			iMaxMemoryCopy,
			mDbQA,
			mDbOnline,
			miOnlineDbCount,
			false
			);

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

bool DBManagerTest::Reload() {
	string mConfigFile = "dbtest.config";

	if( mConfigFile.length() > 0 ) {
		ConfFile* conf = new ConfFile();
		conf->InitConfFile(mConfigFile.c_str(), "");
		if ( !conf->LoadConfFile() ) {
			delete conf;
			return false;
		}

		// DB
		mDbQA.mHost = conf->GetPrivate("DB", "DBHOST", "localhost");
		mDbQA.mPort = atoi(conf->GetPrivate("DB", "DBPORT", "3306").c_str());
		mDbQA.mDbName = conf->GetPrivate("DB", "DBNAME", "qpidnetwork");
		mDbQA.mUser = conf->GetPrivate("DB", "DBUSER", "root");
		mDbQA.mPasswd = conf->GetPrivate("DB", "DBPASS", "123456");
		mDbQA.miMaxDatabaseThread = atoi(conf->GetPrivate("DB", "MAXDATABASETHREAD", "4").c_str());

		miSyncTime = atoi(conf->GetPrivate("DB", "SYNCHRONIZETIME", "30").c_str());
		miSyncOnlineTime = atoi(conf->GetPrivate("DB", "SYNCHRONIZE_ONLINE_TIME", "1").c_str());
		miOnlineDbCount = atoi(conf->GetPrivate("DB", "ONLINE_DB_COUNT", "0").c_str());
		miOnlineDbCount = miOnlineDbCount > 4?4:miOnlineDbCount;
		miOnlineDbCount = miOnlineDbCount < 0?0:miOnlineDbCount;

		char domain[4] = {'\0'};
		for(int i = 0; i < miOnlineDbCount; i++) {
			sprintf(domain, "DB_ONLINE_%d", i);
			mDbOnline[i].mHost = conf->GetPrivate(domain, "DBHOST", "localhost");
			mDbOnline[i].mPort = atoi(conf->GetPrivate(domain, "DBPORT", "3306").c_str());
			mDbOnline[i].mDbName = conf->GetPrivate(domain, "DBNAME", "qpidnetwork_online");
			mDbOnline[i].mUser = conf->GetPrivate(domain, "DBUSER", "root");
			mDbOnline[i].mPasswd = conf->GetPrivate(domain, "DBPASS", "123456");
			mDbOnline[i].miMaxDatabaseThread = atoi(conf->GetPrivate(domain, "MAXDATABASETHREAD", "4").c_str());
			mDbOnline[i].miSiteId = atoi(conf->GetPrivate(domain, "SITEID", "-1").c_str());
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"MatchServer::Reload( "
					"mDbOnline[%d].mHost : %s, "
					"mDbOnline[%d].mPort : %d, "
					"mDbOnline[%d].mDbName : %s, "
					"mDbOnline[%d].mUser : %s, "
					"mDbOnline[%d].mPasswd : %s, "
					"mDbOnline[%d].miMaxDatabaseThread : %d, "
					"mDbOnline[i].miSiteId : %d "
					")",
					i,
					mDbOnline[i].mHost.c_str(),
					i,
					mDbOnline[i].mPort,
					i,
					mDbOnline[i].mDbName.c_str(),
					i,
					mDbOnline[i].mUser.c_str(),
					i,
					mDbOnline[i].mPasswd.c_str(),
					i,
					mDbOnline[i].miMaxDatabaseThread,
					i,
					mDbOnline[i].miSiteId
					);
		}

		delete conf;

		return true;
	}
	return false;
}

//void DBManagerTest::TestSql(string sql) {
//	mDBManager.Init(1, true);
//	bool bResult = false;
//	char** result;
//	int iRow;
//	int iColumn;
//	bResult = mDBManager.Query((char*)sql.c_str(), &result, &iRow, &iColumn);
//
//	if( bResult && result && iRow > 0 ) {
//		for( int j = 1; j < (iRow + 1); j++ ) {
//			for( int k = 0; k < iColumn; k++ ) {
//				printf("%8s |", result[j * iColumn + k]);
//			}
//			printf("\n");
//		}
//	}
//}

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

	unsigned int iHandleTime = 0;
	unsigned int iSingleQueryTime = 0;
	bool bSleepAlready = false;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	iHandleTime = GetTickCount();
	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn);
	if( bResult && result && iRow > 0 ) {
		// 随机起始查询问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;
		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
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

				/*
				 * 查询当前问题相同答案的女士Id集合
				 * 1.超过30个, 随机选取30个
				 * 2.不够30个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= 30 ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = 30;
					iLadyIndex = (rand() % (iNum -iLadyCount));
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
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < 30 ) {
							// 结果集合还不够30个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够30个女士
					if( womanidMap.size() >= 30 ) {
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);

			}

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

			i++;
			i = ((i - 1) % iRow) + 1;

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}
		}
	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}
	iHandleTime = GetTickCount() - iHandleTime;
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

	gettimeofday(&tStart, NULL);

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

	gettimeofday(&tEnd, NULL);
	long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
	usleep(usec);
}

/**
 * 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
 */
bool DBManagerTest::TestQuerySameAnswerLadyList(int index) {
	Json::Value womanListNode;
	const char* pManId = "CM100";
	const char* pSiteId = "1";
	int iQueryIndex = index;

	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	// 执行查询
	char sql[1024] = {'\0'};
	sprintf(sql, "SELECT qid, aid FROM mq_man_answer_%s WHERE manid = '%s';",
			pSiteId,
			pManId
			);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);

	if( bResult && result && iRow > 0 ) {
		// 随机起始男士问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;

		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				int iNum = 0;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND aid = %s AND question_status = 1;",
						pSiteId,
						result[i * iColumn],
						result[i * iColumn + 1]
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;

				/*
				 * 查询当前问题相同答案的女士Id集合
				 * 1.超过30个, 随机选取30个
				 * 2.不够30个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= 30 ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = 30;
					iLadyIndex = (rand() % (iNum -iLadyCount));
				}

				sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
						"WHERE qid = %s AND aid = %s AND question_status = 1 "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						result[i * iColumn],
						result[i * iColumn + 1],
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;

				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < 30 ) {
							// 结果集合还不够30个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够30个女士
					if( womanidMap.size() >= 30 ) {
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			// 对已经获取到的女士统计相同答案问题数量
			iQueryTime = GetTickCount();
			for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
				char** result3 = NULL;
				int iRow3;
				int iColumn3;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE womanid = '%s' AND qid = %s AND aid = %s AND question_status = 1;",
						pSiteId,
						itr->first.c_str(),
						result[i * iColumn],
						result[i * iColumn + 1]
						);

				bResult = mDBManager.Query(sql, &result3, &iRow3, &iColumn3, iQueryIndex);
				if( bResult && result3 && iRow3 > 0 ) {
					itr->second += atoi(result3[1 * iColumn3]);
				}
				mDBManager.FinishQuery(result3);
			}
			iQueryTime = GetTickCount() - iQueryTime;

			i++;
			i = ((i - 1) % iRow) + 1;

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}
		}

		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
			Json::Value womanNode;
			womanNode[itr->first] = itr->second;
			womanListNode.append(womanNode);
		}
	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	return bResult;
}

/**
 * 2.获取跟男士有指定共同问题的女士Id列表接口(http get)
 */
bool DBManagerTest::TestQueryTheSameQuestionLadyList(int index) {
	Json::Value womanListNode;
	const char* pQid = "QA0";
	const char* pSiteId = "1";
	int iQueryIndex = index;

	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();

	// 执行查询
	char sql[1024] = {'\0'};
	string qid = pQid;
	qid = qid.substr(2, qid.length() - 2);
	sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s WHERE qid = %s;",
			pSiteId,
			qid.c_str()
			);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);

	if( bResult && result && iRow > 0 ) {
		int iNum = 0;

		iQueryTime = GetTickCount();
		bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);
		if( bResult && result && iRow > 0 ) {
			iNum = atoi(result[1 * iColumn]);
		}
		iQueryTime = GetTickCount() - iQueryTime;

		char** result2 = NULL;
		int iRow2;
		int iColumn2;

		/*
		 * 查询当前问题相同答案的女士Id集合
		 * 1.超过30个, 随机选取30个
		 * 2.不够30个, 全部选择
		 */
		int iLadyIndex = 0;
		int iLadyCount = 0;
		if( iNum <= 30 ) {
			iLadyCount = iNum;
		} else {
			iLadyCount = 30;
			iLadyIndex = (rand() % (iNum -iLadyCount));
		}

		sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
				"WHERE qid = %s LIMIT %d OFFSET %d;",
				pSiteId,
				qid.c_str(),
				iLadyCount,
				iLadyIndex
				);

		iQueryTime = GetTickCount();
		bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
		iQueryTime = GetTickCount() - iQueryTime;

		if( bResult && result2 && iRow2 > 0 ) {
			for( int j = 1; j < iRow2 + 1; j++ ) {
				// insert womanid
				womanid = result2[j * iColumn2];
				womanListNode.append(womanid);
			}
		}

		mDBManager.FinishQuery(result2);
	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	usleep(1000 * iSingleQueryTime);
	iHandleTime = GetTickCount() - iHandleTime;

	return bResult;
}

/**
 * 3.获取跟男士有任意共同问题的女士Id列表接口(http get)
 */
bool DBManagerTest::TestQueryAnySameQuestionLadyList(int index) {
	Json::Value womanListNode;
	const char* pManId = "CM100";
	const char* pSiteId = "1";
	int iQueryIndex = index;

	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	// 执行查询
	char sql[1024] = {'\0'};
	sprintf(sql, "SELECT qid FROM mq_man_answer_%s "
			"WHERE manid = '%s';",
			pSiteId,
			pManId
			);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mDBManager.Query(sql, &result, &iRow, &iColumn, iQueryIndex);

	if( bResult && result && iRow > 0 ) {
		// 随机起始男士问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;

		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				int iNum = 0;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND question_status = 1;",
						pSiteId,
						result[i * iColumn]
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;

				/*
				 * 查询当前问题相同的女士Id集合
				 * 1.超过30个, 随机选取30个
				 * 2.不够30个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= 30 ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = 30;
					iLadyIndex = (rand() % (iNum -iLadyCount));
				}

				sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
						"WHERE qid = %s AND question_status = 1 "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						result[i * iColumn],
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;

				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < 30 ) {
							// 结果集合还不够30个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够30个女士
					if( womanidMap.size() >= 30 ) {
						break;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			i++;
			i = ((i - 1) % iRow) + 1;

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}
		}

		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
			womanListNode.append(itr->first);
		}
	}
	mDBManager.FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	return bResult;
}

/**
 * 4.获取回答过注册问题的在线女士Id列表接口(http get)
 */
bool DBManagerTest::TestQueryAnySameQuestionOnlineLadyList(int index) {
	Json::Value womanListNode;
	char pQids[1024] = "QA0,QA1,QA2,QA3,QA4,QA5,QA6,QA7";
	const char* pSiteId = "1";
	int iQueryIndex = index;

	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	// 执行查询
	char sql[1024] = {'\0'};

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	char* pQid = NULL;
	char *pFirst = NULL;

	int iCount = 0;
	string qid;

	if( pQids != NULL ) {
		bool bEnougthLady = false;

		int iNum = 0;
		int iMax = 30;

		pQid = strtok_r((char*)pQids, ",", &pFirst);
		while( pQid != NULL && iCount < 7 ) {
			iSingleQueryTime = GetTickCount();

			qid = pQid;
			if(qid.length() > 2) {
				qid = qid.substr(2, qid.length() - 2);
			}

			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM online_woman_%s as o "
						"JOIN mq_woman_answer_%s as m "
						"ON o.womanid = m.womanid "
						"WHERE m.qid = %s"
						";",
						pSiteId,
						pSiteId,
						qid.c_str()
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mDBManager.FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;

				/*
				 * 查询当前问题相同的女士Id集合
				 * 1.超过iMax个, 随机选取iMax个
				 * 2.不够iMax个, 全部选择
				 */
				int iLadyIndex = 0;
				int iLadyCount = 0;
				if( iNum <= iMax ) {
					iLadyCount = iNum;
				} else {
					iLadyCount = iMax;
					iLadyIndex = (rand() % (iNum -iLadyCount));
				}

				sprintf(sql, "SELECT m.womanid FROM online_woman_%s as o "
						"JOIN mq_woman_answer_%s as m "
						"ON o.womanid = m.womanid "
						"WHERE m.qid = %s "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						pSiteId,
						qid.c_str(),
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mDBManager.Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;

				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < 30 ) {
							// 结果集合还不够30个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够iMax个女士
					if( womanidMap.size() >= iMax ) {
						bEnougthLady = true;
					}
				}
				mDBManager.FinishQuery(result2);
			}

			iCount++;

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			if( iSingleQueryTime > 30 ) {
				bSleepAlready = true;
				usleep(1000 * iSingleQueryTime);
			}

			pQid = strtok_r(NULL, ",", &pFirst);
		}

		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
			womanListNode.append(itr->first);
		}
	}

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	return bResult;
}
