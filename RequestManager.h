/*
 * RequestManager.h
 *
 *  Created on: 2015-1-12
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef REQUESTMANAGER_H_
#define REQUESTMANAGER_H_

#include "MessageList.h"
#include "LogManager.h"
#include "DBManager.h"

#include "json/json.h"
#include "md5.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include <map>
using namespace std;

class RequestManager;
class RequestManagerCallback {
public:
	virtual ~RequestManagerCallback(){};
	virtual void OnReload(RequestManager* pRequestManager) = 0;
};

class RequestManager {
public:
	RequestManager();
	virtual ~RequestManager();

	bool Init(DBManager* pDBManager);
	void SetRequestManagerCallback(RequestManagerCallback* pRequestManagerCallback);

	/*
	 *	return : -1:close/0:recv again/1:ok
	 */
	int HandleRecvMessage(Message *m, Message *sm);
	int HandleTimeoutMessage(Message *m, Message *sm);
	int HandleInsideRecvMessage(Message *m, Message *sm);
protected:
	unsigned int GetTickCount();

private:
	DBManager* mpDBManager;
	RequestManagerCallback* mpRequestManagerCallback;
};

#endif /* CLIENTMANAGER_H_ */
