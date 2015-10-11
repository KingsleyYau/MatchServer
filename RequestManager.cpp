/*
 * RequestManager.cpp
 *
 *  Created on: 2015-1-12
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "RequestManager.h"
#include "DataHttpParser.h"

#define UNHANDLE_TIME    5000     // 5秒后丢弃

RequestManager::RequestManager() {
	// TODO Auto-generated constructor stub
}

RequestManager::~RequestManager() {
	// TODO Auto-generated destructor stub
}

bool RequestManager::Init(DBManager* pDBManager) {
	mpDBManager = pDBManager;
	return true;
}

int RequestManager::HandleRecvMessage(Message *m, Message *sm) {
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
	if ( DiffGetTickCount(m->starttime, GetTickCount()) < UNHANDLE_TIME ) {
		if( m->buffer != NULL ) {
			ret = dataHttpParser.ParseData(m->buffer);
		}
	}

	start = GetTickCount();

	if( ret == 1 ) {
		const char* pPath = dataHttpParser.GetPath();
		const char* pManId = dataHttpParser.GetParam("MANID");
		LogManager::GetLogManager()->Log(
				LOG_MSG,
				"RequestManager::HandleRecvMessage( "
				"tid : %d, "
				"m->fd: [%d], "
				"pPath : %s, "
				"manid : %s, "
				"parse "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				pPath,
				pManId
				);

		if( pManId != NULL && strcmp(pPath, "/QUERY") == 0 ) {
			// 执行查询
			char sql[2048] = {'\0'};

//			sprintf(sql,
//					"SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = '%s';",
//					pManId);
			sprintf(sql, "SELECT * FROM man WHERE manid = '%s';", pManId);

			bool bResult = false;
			char** result = NULL;
			int iRow = 0;
			int iColumn = 0;

			char** result2 = NULL;
			int iRow2;
			int iColumn2;
			int qid = 0;
			int aid = 0;

			timeval tStart;
			timeval tEnd;

			map<string, int> womanidMap;
			map<string, int>::iterator itr;
			string womanid;

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
			if( bResult && result && iRow > 0 ) {
				for( int i = 1; i < (iRow + 1); i++ ) {
					gettimeofday(&tStart, NULL);

					qid = atoi(result[i * iColumn + 1]);
					aid = atoi(result[i * iColumn + 4]);

					sprintf(sql, "SELECT womanid FROM woman WHERE qid = %d AND aid = %d AND question_status='1';",
							qid,
							aid
							);
					bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2);
					LogManager::GetLogManager()->Log(
										LOG_STAT,
										"RequestManager::HandleRecvMessage( "
										"tid : %d, "
										"m->fd: [%d], "
										"iRow2 : %d, "
										"iColumn2 : %d "
										")",
										(int)syscall(SYS_gettid),
										m->fd,
										iRow2,
										iColumn2
										);
					if( bResult && result2 && iRow2 > 0 ) {
						for( int k = 1; k < (iRow2 + 1); k++ ) {
							// find womanid
							womanid = result2[k * iColumn2];
							itr = womanidMap.find(womanid);
							if( itr != womanidMap.end() ) {
								itr->second++;
							} else {
								womanidMap.insert(map<string, int>::value_type(womanid, 1));
							}
						}
					}

					mpDBManager->FinishQuery(result2);
					gettimeofday(&tEnd, NULL);
					long usec = (1000 * 1000 * tEnd.tv_sec + tEnd.tv_usec - (1000 * 1000 * tStart.tv_sec + tStart.tv_usec));
					usleep(usec);
					querytime += usec / 1000;
				}

				mpDBManager->FinishQuery(result);

				int size = womanidMap.size();// - 30;
//				size = (size > 0)?size:0;

				int iIndex = 0;//rand() % size;
				int iCount = 0;
				int iItem = 0;
				LogManager::GetLogManager()->Log(
								LOG_STAT,
								"RequestManager::HandleRecvMessage( "
								"tid : %d, "
								"m->fd: [%d], "
								"size : %d, "
								"iIndex : %d "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								size,
								iIndex
								);
				for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
//					iCount++;
//					if( iCount < iIndex ) {
//						continue;
//					}

					iItem++;
					if( iItem > 29 ) {
						break;
					}

					Json::Value womanNode;
					womanNode[itr->first] = itr->second;
					womanListNode.append(womanNode);
				}

				rootSend["womaninfo"] = womanListNode;
			}
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

	time = GetTickCount() - start;
	sm->totaltime = GetTickCount() - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::HandleRecvMessage( "
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

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
								param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}

unsigned int RequestManager::GetTickCount() {
	timeval tNow;
	gettimeofday(&tNow, NULL);
	if (tNow.tv_usec != 0) {
		return (tNow.tv_sec * 1000 + (unsigned int)(tNow.tv_usec / 1000));
	} else{
		return (tNow.tv_sec * 1000);
	}
}
