/*
 * RequestManager.cpp
 *
 *  Created on: 2015-1-12
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "RequestManager.h"
#include "DataHttpParser.h"

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
	if( m == NULL ) {
		return -1;
	}

	int ret = -1;

	DataHttpParser dataHttpParser;
	ret = dataHttpParser.ParseData(m->buffer);

	Json::FastWriter writer;
	Json::Value rootSend;

	const char* pManId = dataHttpParser.GetParam("MANID");
	int iCount = 0;
	int errno = 0;
	char errmsg[1024] = {'\0'};
	unsigned int start = 0;
	unsigned int time = 0;

	if( ret == 1 ) {
		LogManager::GetLogManager()->Log(LOG_MSG, "RequestManager::HandleRecvMessage( "
						"tid : %d, "
						"m->fd: %d, "
						"manid : %s, "
						"parse ok "
						")",
						(int)syscall(SYS_gettid),
						m->fd,
						pManId
						);

		start = GetTickCount();

		if( pManId != NULL ) {
			// 执行查询
			char sql[2048] = {'\0'};
			sprintf(sql,
					"SELECT COUNT(DISTINCT LADY.ID) FROM LADY JOIN MAN ON MAN.QID = LADY.QID AND MAN.AID = LADY.AID WHERE MAN.MANID = '%s';",
					pManId);

			bool bResult = false;
			char** result = NULL;
			int iRow = 0;
			int iColumn = 0;

			bResult = mpDBManager->Query(sql, result, &iRow, &iColumn);
			if( bResult ) {
				if( iRow == 2 ) {
					iCount = atoi(result[1 * iColumn]);
				}
				mpDBManager->FinishQuery(result);
			}
		} else {
			errno = -1;
			sprintf(errmsg, "param not found");
		}

		time = GetTickCount() - start;
		LogManager::GetLogManager()->Log(LOG_MSG, "RequestManager::HandleRecvMessage( "
				"tid : %d, "
				"m->fd: %d, "
				"Query time : %d ms "
				")",
				(int)syscall(SYS_gettid),
				m->fd,
				time
				);

	} else {
		LogManager::GetLogManager()->Log(LOG_MSG, "RequestManager::HandleRecvMessage( "
								"m->fd: %d, "
								"parse fail "
								")",
								m->fd
								);
	}

	rootSend["fd"] = m->fd;
	rootSend["ret"] = ret;
	rootSend["count"] = iCount;
	rootSend["time"] = time;
	rootSend["errno"] = errno;
	rootSend["errmsg"] = errmsg;

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
