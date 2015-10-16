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

class DBManagerTest {
public:
	DBManagerTest();
	virtual ~DBManagerTest();

	void StartTest(int iMaxThread, int iMaxMemoryCopy, int iMaxQuery);
	void TestSql(string sql);

	void Test1(int index);
	void Test2(int index);
	void Test3(int index);
	void Test4(int index);

	DBManager mDBManager;
	int miMaxQuery;

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
