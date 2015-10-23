/*
 * DBManagerTest.h
 *
 *  Created on: 2015-9-24
 *      Author: Max
 */

#ifndef DBMANAGERTEST_H_
#define DBMANAGERTEST_H_

#include "DBManager.h"
#include "KThread.h"
#include "ConfFile.hpp"

class DBManagerTest {
public:
	DBManagerTest();
	virtual ~DBManagerTest();

	void StartTest(int iMaxThread, int iMaxMemoryCopy, int iMaxQuery);
	bool Reload();
//	void TestSql(string sql);

	void Test1(int index);
	void Test2(int index);
	void Test3(int index);
	void Test4(int index);

	/**
	 * 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
	 */
	bool TestQuerySameAnswerLadyList(int index);

	/**
	 * 2.获取跟男士有指定共同问题的女士Id列表接口(http get)
	 */
	bool TestQueryTheSameQuestionLadyList(int index);

	/**
	 * 3.获取跟男士有任意共同问题的女士Id列表接口(http get)
	 */
	bool TestQueryAnySameQuestionLadyList(int index);

	/**
	 * 4.获取跟男士有任意共同问题的在线女士Id列表接口(http get)
	 */
	bool TestQueryAnySameQuestionOnlineLadyList(int index);

	DBManager mDBManager;
	int miMaxQuery;

	// DB
	DBSTRUCT mDbQA;
	int miSyncTime;
	int miSyncOnlineTime;
	int miOnlineDbCount;
	DBSTRUCT mDbOnline[4];

	long miSleep;
	long miQuery;
	int miCur;		// 已经查询次数
	long miMin;		// 最小处理时间
	long miMax;		// 最大处理时间
	long miTol;		// 总处理时间
	long miAvg;		// 平均处理时间
	KMutex mMutex;
};

#endif /* DBMANAGERTEST_H_ */
