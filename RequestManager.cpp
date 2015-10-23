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


RequestManager::RequestManager() {
	// TODO Auto-generated constructor stub
	mpDBManager = NULL;
	mpRequestManagerCallback = NULL;
	miTimeout = 5000;
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

void RequestManager::SetTimeout(unsigned int mSecond) {
	miTimeout = mSecond;
}

int RequestManager::HandleRecvMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend;

	if( m == NULL ) {
		return ret;
	}

	DataHttpParser dataHttpParser;
	if ( DiffGetTickCount(m->starttime, GetTickCount()) < miTimeout ) {
		if( m->buffer != NULL ) {
			ret = dataHttpParser.ParseData(m->buffer);
		}
	}

	if( ret == 1 ) {
		ret = -1;
		const char* pPath = dataHttpParser.GetPath();
		HttpType type = dataHttpParser.GetType();

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"RequestManager::HandleRecvMessage( "
				"tid : %d, "
				"m->fd: [%d], "
				"type : %d, "
				"pPath : %s "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				type,
				pPath
				);

		if( type == GET ) {
			if( strcmp(pPath, "/QUERY_SAME_ANSWER_LADY_LIST") == 0 ) {
				// 1.获取跟男士有任意共同答案的问题的女士Id列表接口(http get)
				const char* pManId = dataHttpParser.GetParam("MANID");
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pManId != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QuerySameAnswerLadyList(womanListNode, pManId, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

			} else if( strcmp(pPath, "/QUERY_THE_SAME_QUESTION_LADY_LIST") == 0 ) {
				// 2.获取跟男士有指定共同问题的女士Id列表接口(http get)
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pQId = dataHttpParser.GetParam("QID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pQId != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QueryTheSameQuestionLadyList(womanListNode, pQId, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

			} else if( strcmp(pPath, "/QUERY_ANY_SAME_QUESTION_LADY_LIST") == 0 ) {
				// 3.获取跟男士有任意共同问题的女士Id列表接口(http get)
				const char* pManId = dataHttpParser.GetParam("MANID");
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pManId != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QueryAnySameQuestionLadyList(womanListNode, pManId, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

			} else if( strcmp(pPath, "/QUERY_ANY_SAME_QUESTION_ONLINE_LADY_LIST") == 0 ) {
				// 4.获取回答过注册问题的在线女士Id列表接口(http get)
				const char* pQIds = dataHttpParser.GetParam("QIDS");
				const char* pSiteId = dataHttpParser.GetParam("SITEID");
				const char* pLimit = dataHttpParser.GetParam("LIMIT");

				if( (pQIds != NULL) && (pSiteId != NULL) ) {
					Json::Value womanListNode;
					if( QueryAnySameQuestionOnlineLadyList(womanListNode, pQIds, pSiteId, pLimit, m) ) {
						ret = 1;
						rootSend["womaninfo"] = womanListNode;
					}
				}

			}
		}
	}

	sm->totaltime = GetTickCount() - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::HandleRecvMessage( "
			"tid : %d, "
			"m->fd: [%d], "
			"iTotaltime : %u ms, "
			"ret : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			sm->totaltime,
			ret
			);

//	rootSend["fd"] = m->fd;
//	rootSend["iTotaltime"] = sm->totaltime;
//	rootSend["womaninfo"] = womanListNode;

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
			(int)param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}

int RequestManager::HandleTimeoutMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	unsigned int iHandleTime = 0;

	if( m == NULL ) {
		return ret;
	}

	iHandleTime = GetTickCount();

	DataHttpParser dataHttpParser;
	if( m->buffer != NULL ) {
		ret = dataHttpParser.ParseData(m->buffer);
	}

	if( ret == 1 ) {

	}

	iHandleTime = GetTickCount() - iHandleTime;
	sm->totaltime = GetTickCount() - m->starttime;
	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::HandleTimeoutMessage( "
			"tid : %d, "
			"m->fd: [%d], "
			"iHandleTime : %u ms, "
			"iTotaltime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iHandleTime,
			sm->totaltime
			);

//	rootSend["fd"] = m->fd;
//	rootSend["iTotaltime"] = sm->totaltime;


	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
			(int)param.length(), param.c_str());
	sm->len = strlen(sm->buffer);

	return ret;
}

int RequestManager::HandleInsideRecvMessage(Message *m, Message *sm) {
	int ret = -1;

	Json::FastWriter writer;
	Json::Value rootSend, womanListNode, womanNode;

	unsigned int iHandleTime = 0;

	if( m == NULL ) {
		return ret;
	}

	iHandleTime = GetTickCount();

	DataHttpParser dataHttpParser;
	if( m->buffer != NULL ) {
		ret = dataHttpParser.ParseData(m->buffer);
	}

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
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"RequestManager::HandleInsideRecvMessage( "
					"tid : %d, "
					"m->fd : [%d], "
					"start "
					")",
					(int)syscall(SYS_gettid),
					m->fd
					);

			mpDBManager->SyncForce();
		} else if( strcmp(pPath, "/RELOAD") == 0 ) {
			LogManager::GetLogManager()->Log(
					LOG_MSG,
					"RequestManager::HandleInsideRecvMessage( "
					"tid : %d, "
					"m->fd : [%d], "
					"start "
					")",
					(int)syscall(SYS_gettid),
					m->fd
					);
			if( mpRequestManagerCallback != NULL ) {
				mpRequestManagerCallback->OnReload(this);
			}
		} else if( strcmp(pPath, "/QUERY") == 0 ) {
//			char* sql = (char*)dataHttpParser.GetParam("SQL");
//			bool bResult = false;
//			char** result = NULL;
//			int iRow = 0;
//			int iColumn = 0;
//
//			Json::Value jsonNode, jsonList;
//			char row[32];
//			string column = "";
//
//			bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn);
//			LogManager::GetLogManager()->Log(
//					LOG_STAT,
//					"RequestManager::HandleInsideRecvMessage( "
//					"tid : %d, "
//					"m->fd: [%d], "
//					"iRow : %d, "
//					"iColumn : %d "
//					")",
//					(int)syscall(SYS_gettid),
//					m->fd,
//					iRow,
//					iColumn
//					);
//			if( bResult && result && iRow > 0 ) {
//				for( int i = 1; i < (iRow + 1); i++ ) {
//					sprintf(row, "%d", i);
//					column = "";
//					for( int j = 0; j < iColumn; j++ ) {
//						column += result[i * iColumn + j];
//						column += ",";
//					}
//					column = column.substr(0, column.length() -1);
//					jsonNode[row] = column;
//					jsonList.append(jsonNode);
//				}
//			}
//
//			rootSend["result"] = jsonList;
		} else {
			ret = -1;
		}
	}

	rootSend["ret"] = ret;

	string param = writer.write(rootSend);

	snprintf(sm->buffer, MAXLEN - 1, "HTTP/1.1 200 ok\r\nContext-Length:%d\r\n\r\n%s",
			(int)param.length(), param.c_str()
			);
	sm->len = strlen(sm->buffer);

	return ret;
}

bool RequestManager::QuerySameAnswerLadyList(
		Json::Value& womanListNode,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QuerySameAnswerLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pManId,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};
	sprintf(sql, "SELECT qid, aid FROM mq_man_answer_%s WHERE manid = '%s';", pSiteId, pManId);

	bool bResult = false;
	char** result = NULL;
	int iRow = 0;
	int iColumn = 0;

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QuerySameAnswerLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iRow,
			iColumn,
			iQueryIndex
			);

	if( bResult && result && iRow > 0 ) {
		// 随机起始男士问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;

		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}
		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND aid = %s AND question_status = 1;",
						pSiteId,
						result[i * iColumn],
						result[i * iColumn + 1]
						);

				iQueryTime = GetTickCount();
				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mpDBManager->FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"RequestManager::QuerySameAnswerLadyList( "
									"tid : %d, "
									"m->fd: [%d], "
									"Count iQueryTime : %u m, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
									m->fd,
									iQueryTime,
									iQueryIndex,
									iNum,
									iMax
									);

				/*
				 * 查询当前问题相同答案的女士Id集合
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
				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"RequestManager::QuerySameAnswerLadyList( "
									"tid : %d, "
									"m->fd: [%d], "
									"Query iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iRow2 : %d, "
									"iColumn2 : %d, "
									"iLadyCount : %d, "
									"iLadyIndex : %d, "
									"womanidMap.size() : %d "
									")",
									(int)syscall(SYS_gettid),
									m->fd,
									iQueryTime,
									iQueryIndex,
									iRow2,
									iColumn2,
									iLadyCount,
									iLadyIndex,
									womanidMap.size()
									);


				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < iMax ) {
							// 结果集合还不够iMax个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够iMax个女士
					if( womanidMap.size() >= iMax ) {
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"RequestManager::QuerySameAnswerLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iMax
								);
						bEnougthLady = true;
					}
				}
				mpDBManager->FinishQuery(result2);
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

				bResult = mpDBManager->Query(sql, &result3, &iRow3, &iColumn3, iQueryIndex);
				if( bResult && result3 && iRow3 > 0 ) {
					itr->second += atoi(result3[1 * iColumn3]);
				}
				mpDBManager->FinishQuery(result3);
			}

			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
			iQueryTime = GetTickCount() - iQueryTime;
			LogManager::GetLogManager()->Log(
								LOG_STAT,
								"RequestManager::QuerySameAnswerLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"Double Check iQueryTime : %u ms, "
								"iQueryIndex : %d, "
								"iSingleQueryTime : %u ms "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iQueryTime,
								iQueryIndex,
								iSingleQueryTime
								);

			i++;
			i = ((i - 1) % iRow) + 1;

			// 单词请求是否大于30ms
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
	mpDBManager->FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::QuerySameAnswerLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

bool RequestManager::QueryTheSameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pQid,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"qid : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pQid,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};

	string qid = pQid;
	if(qid.length() > 2) {
		qid = qid.substr(2, qid.length() - 2);
	}

	sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
			"WHERE qid = %s;",
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

	bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iRow,
			iColumn,
			iQueryIndex
			);

	if( bResult && result && iRow > 0 ) {
		char** result2 = NULL;
		int iRow2;
		int iColumn2;

		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		iQueryTime = GetTickCount();
		bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
		if( bResult && result2 && iRow > 0 ) {
			iNum = atoi(result2[1 * iColumn2]);
		}
		mpDBManager->FinishQuery(result2);
		iQueryTime = GetTickCount() - iQueryTime;

		LogManager::GetLogManager()->Log(
							LOG_STAT,
							"RequestManager::QueryTheSameQuestionLadyList( "
							"tid : %d, "
							"m->fd: [%d], "
							"Count iQueryTime : %u ms, "
							"iQueryIndex : %d, "
							"iNum : %d, "
							"iMax : %d "
							")",
							(int)syscall(SYS_gettid),
							m->fd,
							iQueryTime,
							iQueryIndex,
							iNum,
							iMax
							);

		/*
		 * 查询当前问题相同答案的女士Id集合
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

		sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
				"WHERE qid = %s "
				"LIMIT %d OFFSET %d;",
				pSiteId,
				qid.c_str(),
				iLadyCount,
				iLadyIndex
				);

		iQueryTime = GetTickCount();
		bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
		iQueryTime = GetTickCount() - iQueryTime;

		if( bResult && result2 && iRow2 > 0 ) {
			for( int j = 1; j < iRow2 + 1; j++ ) {
				// insert womanid
				womanid = result2[j * iColumn2];
				womanListNode.append(womanid);
			}
		}

		mpDBManager->FinishQuery(result2);

		LogManager::GetLogManager()->Log(
							LOG_STAT,
							"RequestManager::QueryTheSameQuestionLadyList( "
							"tid : %d, "
							"m->fd: [%d], "
							"Query iQueryTime : %u ms, "
							"iQueryIndex : %d, "
							"iRow2 : %d, "
							"iColumn2 : %d, "
							"iLadyCount : %d, "
							"iLadyIndex : %d "
							")",
							(int)syscall(SYS_gettid),
							m->fd,
							iQueryTime,
							iQueryIndex,
							iRow2,
							iColumn2,
							iLadyCount,
							iLadyIndex
							);


	}
	mpDBManager->FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	usleep(1000 * iSingleQueryTime);
	iHandleTime = GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::QueryTheSameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iHandleTime
			);

	return bResult;
}

bool RequestManager::QueryAnySameQuestionLadyList(
		Json::Value& womanListNode,
		const char* pManId,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"manid : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pManId,
			pSiteId,
			pLimit
			);

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

	bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn, iQueryIndex);
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"iRow : %d, "
			"iColumn : %d, "
			"iQueryIndex : %d "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			iRow,
			iColumn,
			iQueryIndex
			);

	if( bResult && result && iRow > 0 ) {
		// 随机起始男士问题位置
		int iManIndex = (rand() % iRow) + 1;
		bool bEnougthLady = false;

		int iNum = 0;

		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
			iSingleQueryTime = GetTickCount();
			if( !bEnougthLady ) {
				// 查询当前问题相同答案的女士数量
				char** result2 = NULL;
				int iRow2;
				int iColumn2;

				sprintf(sql, "SELECT count(*) FROM mq_woman_answer_%s "
						"WHERE qid = %s AND question_status = 1;",
						pSiteId,
						result[i * iColumn]
						);

				iQueryTime = GetTickCount();
				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mpDBManager->FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"RequestManager::QueryAnySameQuestionLadyList( "
						"tid : %d, "
						"m->fd: [%d], "
						"Count iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iNum : %d, "
						"iMax : %d "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						iQueryTime,
						iQueryIndex,
						iNum,
						iMax
						);

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

				sprintf(sql, "SELECT womanid FROM mq_woman_answer_%s "
						"WHERE qid = %s AND question_status = 1 "
						"LIMIT %d OFFSET %d;",
						pSiteId,
						result[i * iColumn],
						iLadyCount,
						iLadyIndex
						);

				iQueryTime = GetTickCount();
				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"RequestManager::QueryAnySameQuestionLadyList( "
						"tid : %d, "
						"m->fd: [%d], "
						"Query iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iRow2 : %d, "
						"iColumn2 : %d, "
						"iLadyCount : %d, "
						"iLadyIndex : %d, "
						"womanidMap.size() : %d "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						iQueryTime,
						iQueryIndex,
						iRow2,
						iColumn2,
						iLadyCount,
						iLadyIndex,
						womanidMap.size()
						);

				if( bResult && result2 && iRow2 > 0 ) {
					for( int j = 1; j < iRow2 + 1; j++ ) {
						// insert womanid
						womanid = result2[j * iColumn2];
						if( womanidMap.size() < iMax ) {
							// 结果集合还不够iMax个女士, 插入
							womanidMap.insert(map<string, int>::value_type(womanid, 0));
						} else {
							// 已满
							break;
						}
					}

					// 标记为已经获取够iMax个女士
					if( womanidMap.size() >= iMax ) {
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"RequestManager::QueryAnySameQuestionLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iMax
								);
						bEnougthLady = true;
					}
				}
				mpDBManager->FinishQuery(result2);
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
	mpDBManager->FinishQuery(result);

	iSingleQueryTime = GetTickCount() - iHandleTime;
	if( !bSleepAlready ) {
		usleep(1000 * iSingleQueryTime);
	}

	iHandleTime =  GetTickCount() - iHandleTime;

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::QueryAnySameQuestionLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}

//bool RequestManager::QueryAnySameQuestionOnlineLadyList(
//		Json::Value& womanListNode,
//		const char* pManId,
//		const char* pSiteId,
//		const char* pLimit,
//		Message *m
//		) {
//	unsigned int iQueryTime = 0;
//	unsigned int iSingleQueryTime = 0;
//	unsigned int iHandleTime = GetTickCount();
//	bool bSleepAlready = false;
//
//	Json::Value womanNode;
//
//	int iQueryIndex = m->index;
//
//	LogManager::GetLogManager()->Log(
//			LOG_STAT,
//			"RequestManager::QueryAnySameQuestionOnlineLadyList( "
//			"tid : %d, "
//			"m->fd: [%d], "
//			"manid : %s, "
//			"siteid : %s, "
//			"limit : %s "
//			")",
//			(int)syscall(SYS_gettid),
//			m->fd,
//			pManId,
//			pSiteId,
//			pLimit
//			);
//
//	// 执行查询
//	char sql[1024] = {'\0'};
//	sprintf(sql, "SELECT qid FROM mq_man_answer WHERE manid = '%s';", pManId);
//
//	bool bResult = false;
//	char** result = NULL;
//	int iRow = 0;
//	int iColumn = 0;
//
//	map<string, int> womanidMap;
//	map<string, int>::iterator itr;
//	string womanid;
//
//	bResult = mpDBManager->Query(sql, &result, &iRow, &iColumn, iQueryIndex);
//	LogManager::GetLogManager()->Log(
//			LOG_STAT,
//			"RequestManager::QueryAnySameQuestionOnlineLadyList( "
//			"tid : %d, "
//			"m->fd: [%d], "
//			"iRow : %d, "
//			"iColumn : %d, "
//			"iQueryIndex : %d "
//			")",
//			(int)syscall(SYS_gettid),
//			m->fd,
//			iRow,
//			iColumn,
//			iQueryIndex
//			);
//
//	if( bResult && result && iRow > 0 ) {
//		// 随机起始男士问题位置
//		int iManIndex = (rand() % iRow) + 1;
//		bool bEnougthLady = false;
//		int iNum = 0;
//		int iMax = 0;
//		if( pLimit != NULL ) {
//			iMax = atoi(pLimit);
//		}
//
//		iMax = (iMax < 4)?4:iMax;
//		iMax = (iMax > 30)?30:iMax;
//
//		for( int i = iManIndex, iCount = 0; iCount < iRow; iCount++ ) {
//			iSingleQueryTime = GetTickCount();
//			if( !bEnougthLady ) {
//				// 查询当前问题相同答案的女士数量
//				char** result2 = NULL;
//				int iRow2;
//				int iColumn2;
//
//				sprintf(sql, "SELECT count(*) FROM online_woman JOIN mq_woman_answer "
//						"ON online_woman.womanid = mq_woman_answer.womanid "
//						"WHERE mq_woman_answer.qid = %s AND mq_woman_answer.siteid = %s "
//						";",
//						result[i * iColumn],
//						pSiteId
//						);
//
//				iQueryTime = GetTickCount();
//				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
//				if( bResult && result2 && iRow2 > 0 ) {
//					iNum = atoi(result2[1 * iColumn2]);
//				}
//				mpDBManager->FinishQuery(result2);
//
//				iQueryTime = GetTickCount() - iQueryTime;
//				LogManager::GetLogManager()->Log(
//									LOG_STAT,
//									"RequestManager::QueryAnySameQuestionOnlineLadyList( "
//									"tid : %d, "
//									"m->fd: [%d], "
//									"Count iQueryTime : %d, "
//									"iQueryIndex : %d, "
//									"iNum : %d, "
//									"iMax : %d "
//									")",
//									(int)syscall(SYS_gettid),
//									m->fd,
//									iQueryTime,
//									iQueryIndex,
//									iNum,
//									iMax
//									);
//
//				/*
//				 * 查询当前问题相同的女士Id集合
//				 * 1.超过iMax个, 随机选取iMax个
//				 * 2.不够iMax个, 全部选择
//				 */
////				int iLadyIndex = 0;
////				int iLadyCount = 0;
////				if( mpDBManager->GetLastOnlineLadyRecordId() <= iMax ) {
////					iLadyCount = mpDBManager->GetLastOnlineLadyRecordId();
////				} else {
////					iLadyCount = iMax;
////					iLadyIndex = (rand() % (mpDBManager->GetLastOnlineLadyRecordId() - iMax));
////				}
//
//				int iLadyIndex = 0;
//				int iLadyCount = 0;
//				if( iNum <= iMax ) {
//					iLadyCount = iNum;
//				} else {
//					iLadyCount = iMax;
//					iLadyIndex = (rand() % (iNum -iLadyCount));
//				}
//
//				sprintf(sql, "SELECT mq_woman_answer.womanid FROM mq_woman_answer JOIN online_woman "
//						"ON mq_woman_answer.womanid = online_woman.womanid "
//						"WHERE mq_woman_answer.qid = %s AND mq_woman_answer.siteid = %s "
//						"LIMIT %d OFFSET %d;",
//						result[i * iColumn],
//						pSiteId,
//						iLadyCount,
//						iLadyIndex
//						);
//
//				iQueryTime = GetTickCount();
//				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
//				iQueryTime = GetTickCount() - iQueryTime;
//				LogManager::GetLogManager()->Log(
//						LOG_STAT,
//						"RequestManager::QueryAnySameQuestionOnlineLadyList( "
//						"tid : %d, "
//						"m->fd: [%d], "
//						"Query iQueryTime : %d, "
//						"iQueryIndex : %d, "
//						"iRow2 : %d, "
//						"iColumn2 : %d, "
//						"iLadyCount : %d, "
//						"iLadyIndex : %d, "
//						"womanidMap.size() : %d "
//						")",
//						(int)syscall(SYS_gettid),
//						m->fd,
//						iQueryTime,
//						iQueryIndex,
//						iRow2,
//						iColumn2,
//						iLadyCount,
//						iLadyIndex,
//						womanidMap.size()
//						);
//
//				if( bResult && result2 && iRow2 > 0 ) {
//					for( int j = 1; j < iRow2 + 1; j++ ) {
//						// insert womanid
//						womanid = result2[j * iColumn2];
//						if( womanidMap.size() < 30 ) {
//							// 结果集合还不够30个女士, 插入
//							womanidMap.insert(map<string, int>::value_type(womanid, 0));
//						} else {
//							// 已满
//							break;
//						}
//					}
//
//					// 标记为已经获取够iMax个女士
//					if( womanidMap.size() >= iMax ) {
//						LogManager::GetLogManager()->Log(
//								LOG_STAT,
//								"RequestManager::QueryAnySameQuestionOnlineLadyList( "
//								"tid : %d, "
//								"m->fd: [%d], "
//								"womanidMap.size() >= %d break "
//								")",
//								(int)syscall(SYS_gettid),
//								m->fd,
//								iMax
//								);
//						break;
//					}
//				}
//				mpDBManager->FinishQuery(result2);
//			}
//
//			i++;
//			i = ((i - 1) % iRow) + 1;
//
//			iSingleQueryTime = GetTickCount() - iSingleQueryTime;
//			if( iSingleQueryTime > 30 ) {
//				bSleepAlready = true;
//				usleep(1000 * iSingleQueryTime);
//			}
//		}
//
//		for( itr = womanidMap.begin(); itr != womanidMap.end(); itr++ ) {
//			womanListNode.append(itr->first);
//		}
//	}
//	mpDBManager->FinishQuery(result);
//
//	iSingleQueryTime = GetTickCount() - iHandleTime;
//	if( !bSleepAlready ) {
//		usleep(1000 * iSingleQueryTime);
//	}
//
//	iHandleTime =  GetTickCount() - iHandleTime;
//
//	LogManager::GetLogManager()->Log(
//			LOG_MSG,
//			"RequestManager::QueryAnySameQuestionOnlineLadyList( "
//			"tid : %d, "
//			"m->fd: [%d], "
//			"bSleepAlready : %s, "
//			"iHandleTime : %u ms "
//			")",
//			(int)syscall(SYS_gettid),
//			m->fd,
//			bSleepAlready?"true":"false",
//			iHandleTime
//			);
//
//	return bResult;
//}
bool RequestManager::QueryAnySameQuestionOnlineLadyList(
		Json::Value& womanListNode,
		const char* pQids,
		const char* pSiteId,
		const char* pLimit,
		Message *m
		) {
	unsigned int iQueryTime = 0;
	unsigned int iSingleQueryTime = 0;
	unsigned int iHandleTime = GetTickCount();
	bool bSleepAlready = false;

	Json::Value womanNode;

	int iQueryIndex = m->index;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"RequestManager::QueryAnySameQuestionOnlineLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"qids : %s, "
			"siteid : %s, "
			"limit : %s "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			pQids,
			pSiteId,
			pLimit
			);

	// 执行查询
	char sql[1024] = {'\0'};

	map<string, int> womanidMap;
	map<string, int>::iterator itr;
	string womanid;

	char* pQid = NULL;
	char *pFirst = NULL;

	bool bResult = false;
	int iCount = 0;
	string qid;

	if( pQids != NULL ) {
		bool bEnougthLady = false;

		int iNum = 0;
		int iMax = 0;
		if( pLimit != NULL ) {
			iMax = atoi(pLimit);
		}

		iMax = (iMax < 4)?4:iMax;
		iMax = (iMax > 30)?30:iMax;

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
				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				if( bResult && result2 && iRow2 > 0 ) {
					iNum = atoi(result2[1 * iColumn2]);
				}
				mpDBManager->FinishQuery(result2);

				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
									LOG_STAT,
									"RequestManager::QueryAnySameQuestionOnlineLadyList( "
									"tid : %d, "
									"m->fd: [%d], "
									"Count iQueryTime : %u ms, "
									"iQueryIndex : %d, "
									"iNum : %d, "
									"iMax : %d "
									")",
									(int)syscall(SYS_gettid),
									m->fd,
									iQueryTime,
									iQueryIndex,
									iNum,
									iMax
									);

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
				bResult = mpDBManager->Query(sql, &result2, &iRow2, &iColumn2, iQueryIndex);
				iQueryTime = GetTickCount() - iQueryTime;
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"RequestManager::QueryAnySameQuestionOnlineLadyList( "
						"tid : %d, "
						"m->fd: [%d], "
						"Query iQueryTime : %u ms, "
						"iQueryIndex : %d, "
						"iRow2 : %d, "
						"iColumn2 : %d, "
						"iLadyCount : %d, "
						"iLadyIndex : %d, "
						"womanidMap.size() : %d "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						iQueryTime,
						iQueryIndex,
						iRow2,
						iColumn2,
						iLadyCount,
						iLadyIndex,
						womanidMap.size()
						);

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
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"RequestManager::QueryAnySameQuestionOnlineLadyList( "
								"tid : %d, "
								"m->fd: [%d], "
								"womanidMap.size() >= %d break "
								")",
								(int)syscall(SYS_gettid),
								m->fd,
								iMax
								);
						bEnougthLady = true;
					}
				}
				mpDBManager->FinishQuery(result2);
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

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"RequestManager::QueryAnySameQuestionOnlineLadyList( "
			"tid : %d, "
			"m->fd: [%d], "
			"bSleepAlready : %s, "
			"iHandleTime : %u ms "
			")",
			(int)syscall(SYS_gettid),
			m->fd,
			bSleepAlready?"true":"false",
			iHandleTime
			);

	return bResult;
}
