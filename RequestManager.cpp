/*
 * RequestManager.cpp
 *
 *  Created on: 2015-1-12
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "RequestManager.h"
#include "DataHttpParser.h"
#include "TimeProc.hpp"

#include <algorithm>

#define UNHANDLE_TIME    5000     // 5秒后丢弃

RequestManager::RequestManager() {
	// TODO Auto-generated constructor stub
	mpRequestManagerCallback = NULL;
}

RequestManager::~RequestManager() {
	// TODO Auto-generated destructor stub
}

bool RequestManager::Init(DBManager* pDBManager) {
	mpDBManager = pDBManager;
	return true;
}

void RequestManager::SetRequestManagerCallback(RequestManagerCallback* pRequestManagerCallback) {
	mpRequestManagerCallback = pRequestManagerCallback;
}

int RequestManager::HandleRecvMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	unsigned int start = 0;
	unsigned int iHandleTime = 0;
	unsigned int iQueryTime = 0;

	if( m == NULL ) {
		return ret;
	}

	DataHttpParser dataHttpParser;
	if ( DiffGetTickCount(m->starttime, GetTickCount()) < UNHANDLE_TIME ) {
		if( m->buffer != NULL ) {
			ret = dataHttpParser.ParseData(m->buffer);
		}
	}

	start = GetTickCount();

	if( ret == 1 ) {
		const char* pPath = dataHttpParser.GetPath();
		const char* pManId = dataHttpParser.GetParam("MANID");
		const char* pSiteId = dataHttpParser.GetParam("SITEID");
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"RequestManager::HandleRecvMessage( "
				"tid : %d, "
				"m->fd: [%d], "
				"pPath : %s, "
				"manid : %s, "
				"siteid : %s, "
				"parse "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				pPath,
				pManId,
				pSiteId
				);

		if( pManId != NULL && strcmp(pPath, "/QUERY") == 0 ) {
			// 执行查询
			char sql[1024] = {'\0'};
			long long qid = 0;
			int aid = 0;
			int siteid = 0;
			string womanIds = "";
			string where = "";

//			sprintf(sql,
//					"SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = '%s';",
//					pManId);
			sprintf(sql, "SELECT qid, aid FROM man WHERE manid = '%s';", pManId);
			if( pSiteId != NULL ) {
				siteid = atoi(pSiteId);
			}

			bool bResult = false;
			char** result = NULL;
			int iRow = 0;
			int iColumn = 0;

			timeval tStart;
			timeval tEnd;

			map<string, int> womanidMap;
			map<string, int>::iterator itr;
			string womanid;

			iQueryTime = GetTickCount();
			bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn);
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"RequestManager::HandleRecvMessage( "
					"tid : %d, "
					"m->fd: [%d], "
					"iRow : %d, "
					"iColumn : %d "
					")",
					(int)syscall(SYS_gettid),
					m->fd,
					iRow,
					iColumn
					);

			gettimeofday(&tStart, NULL);

			if( bResult && result && iRow > 0 ) {
				// 随机起始查询问题位置
				int iManIndex = (rand() % iRow) + 1;
				bool bEnougthLady = false;
				for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {

//					LogManager::GetLogManager()->Log(
//										LOG_STAT,
//										"RequestManager::HandleRecvMessage( "
//										"tid : %d, "
//										"m->fd: [%d], "
//										"i : %d "
//										")",
//										(int)syscall(SYS_gettid),
//										m->fd,
//										i
//										);

					if( !bEnougthLady ) {
						// query more lady
						sprintf(sql, "SELECT womanid FROM woman WHERE qid = %s AND aid = %s AND question_status = 1 AND siteid = %d;",
								result[i * iColumn],
								result[i * iColumn + 1],
								siteid
								);

						char** result2 = NULL;
						int iRow2;
						int iColumn2;

						bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2);
//						LogManager::GetLogManager()->Log(
//											LOG_STAT,
//											"RequestManager::HandleRecvMessage( "
//											"tid : %d, "
//											"m->fd: [%d], "
//											"iManIndex : %d, "
//											"iRow2 : %d, "
//											"iColumn2 : %d "
//											")",
//											(int)syscall(SYS_gettid),
//											m->fd,
//											iManIndex,
//											iRow2,
//											iColumn2
//											);
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
//							LogManager::GetLogManager()->Log(
//												LOG_STAT,
//												"RequestManager::HandleRecvMessage( "
//												"tid : %d, "
//												"m->fd: [%d], "
//												"iLadyIndex : %d "
//												")",
//												(int)syscall(SYS_gettid),
//												m->fd,
//												iLadyIndex
//												);
							for( int j = iLadyIndex, k = 0; (j < iRow2 + 1) && (k < iLadyCount); k++, j++ ) {
								// find womanid
								womanid = result2[j * iColumn2];
								womanidMap.insert(map<string, int>::value_type(womanid, 1));
							}
						}
						mpDBManager->FinishQuery(result2);
					} else {
						for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
							char** result3 = NULL;
							int iRow3;
							int iColumn3;

							sprintf(sql, "SELECT count(*) FROM woman WHERE womanid = '%s' AND qid = %s AND aid = %s AND question_status = 1 AND siteid = %d;",
									itr->first.c_str(),
									result[i * iColumn],
									result[i * iColumn + 1],
									siteid
									);

							bResult = mpDBManager->Query(sql, &result3, &iRow3, &iColumn3);
//							LogManager::GetLogManager()->Log(
//												LOG_STAT,
//												"RequestManager::HandleRecvMessage( "
//												"tid : %d, "
//												"m->fd: [%d], "
//												"iRow3 : %d, "
//												"iColumn3 : %d, "
//												")",
//												(int)syscall(SYS_gettid),
//												m->fd,
//												iRow3,
//												iColumn3
//												);
							if( bResult && result3 && iRow3 > 0 ) {
								LogManager::GetLogManager()->Log(
													LOG_STAT,
													"RequestManager::HandleRecvMessage( "
													"tid : %d, "
													"m->fd: [%d], "
													"result3[1 * iColumn3] : %s, "
													"itr->second : %d "
													")",
													(int)syscall(SYS_gettid),
													m->fd,
													result3[1 * iColumn3],
													itr->second
													);
								if( strcmp(result3[1 * iColumn3], "0") != 0 ) {
									itr->second++;
								}
							}
							mpDBManager->FinishQuery(result3);
						}
					}

					i++;
					i = ((i - 1) % iRow) + 1;
				}

				gettimeofday(&tEnd, NULL);
				long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
				usleep(usec);

				iQueryTime = GetTickCount() - iQueryTime;
				rootSend["iQueryTime"] = iQueryTime;

				for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
					Json::Value womanNode;
					womanNode[itr->first] = itr->second;
					womanListNode.append(womanNode);
				}
			}
			mpDBManager->FinishQuery(result);
		} else {
			ret = -1;
		}
	} else {
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"RequestManager::HandleRecvMessage( "
				"m->fd: [%d], "
				"parse fail "
				")",
				m->fd
				);
	}

	int iEnd = GetTickCount();
	iHandleTime = iEnd - start;
	sm->totaltime = iEnd - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::HandleRecvMessage( "
			"tid : %d, "
			"m->fd: [%d], "
			"iQueryTime : %u ms, "
			"iHandleTime : %u ms, "
			"totaltime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iQueryTime,
			iHandleTime,
			sm->totaltime
			);

	rootSend["fd"] = m->fd;
	rootSend["iHandleTime"] = iHandleTime;
	rootSend["totaltime"] = sm->totaltime;
	rootSend["womaninfo"] = womanListNode;

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
								param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}

int RequestManager::HandleTimeoutMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	unsigned int start = 0;
	unsigned int time = 0;
	unsigned int querytime = 0;

	if( m == NULL ) {
		return ret;
	}

	DataHttpParser dataHttpParser;
	if( m->buffer != NULL ) {
		ret = dataHttpParser.ParseData(m->buffer);
	}

	start = GetTickCount();

	if( ret == 1 ) {

	}

	time = GetTickCount() - start;
	sm->totaltime = GetTickCount() - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::HandleTimeoutMessage( "
			"tid : %d, "
			"m->fd: [%d], "
			"querytime : %u ms, "
			"time : %u ms, "
			"totaltime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			time,
			sm->totaltime
			);
	rootSend["fd"] = m->fd;
	rootSend["querytime"] = querytime;
	rootSend["time"] = time;
	rootSend["totaltime"] = sm->totaltime;
	rootSend["womaninfo"] = womanListNode;

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
								param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}

int RequestManager::HandleInsideRecvMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	unsigned int start = 0;
	unsigned int time = 0;
	unsigned int querytime = 0;

	if( m == NULL ) {
		return ret;
	}

	DataHttpParser dataHttpParser;
	if( m->buffer != NULL ) {
		ret = dataHttpParser.ParseData(m->buffer);
	}

	start = GetTickCount();

	if( ret == 1 ) {
		const char* pPath = dataHttpParser.GetPath();
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"RequestManager::HandleInsideRecvMessage( "
				"tid : %d, "
				"m->fd: [%d], "
				"pPath : %s, "
				"parse "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				pPath
				);

		if( strcmp(pPath, "/SYNC") == 0 ) {
			mpDBManager->SyncForce();
		} else if( strcmp(pPath, "/RELOAD") == 0 ) {
			if( mpRequestManagerCallback != NULL ) {
				mpRequestManagerCallback->OnReload(this);
			}
		} else if( strcmp(pPath, "/QUERY") == 0 ) {
			char* sql = (char*)dataHttpParser.GetParam("SQL");
			bool bResult = false;
			char** result = NULL;
			int iRow = 0;
			int iColumn = 0;

			Json::Value jsonNode, jsonList;
			char row[32];
			string column = "";

			bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn);
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"RequestManager::HandleInsideRecvMessage( "
					"tid : %d, "
					"m->fd: [%d], "
					"iRow : %d, "
					"iColumn : %d "
					")",
					(int)syscall(SYS_gettid),
					m->fd,
					iRow,
					iColumn
					);
			if( bResult && result && iRow > 0 ) {
				for( int i = 1; i < (iRow + 1); i++ ) {
					sprintf(row, "%d", i);
					column = "";
					for( int j = 0; j < iColumn; j++ ) {
						column += result[i * iColumn + j];
						column += ",";
					}
					column = column.substr(0, column.length() -1);
					jsonNode[row] = column;
					jsonList.append(jsonNode);
				}
			}

			rootSend["result"] = jsonList;
		} else {
			ret = -1;
		}
	}

	rootSend["fd"] = m->fd;
	rootSend["ret"] = ret;

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
								param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}
