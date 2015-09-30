/*
 * LogManager.h
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef LOGMANAGER_H_
#define LOGMANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MessageList.h"
#include "KThread.h"
#include "KLog.h"
#include "LogFile.hpp"

class LogManager {
public:

	static LogManager *GetLogManager();

	static void LogSetFlushBuffer(unsigned int iLen);
	static void LogFlushMem2File();

	LogManager();
	virtual ~LogManager();

	bool Start(int maxIdle, LOG_LEVEL nLevel = LOG_STAT);
	bool Stop();
	bool IsRunning();
	bool Log(LOG_LEVEL nLevel, const char *format, ...);
	int MkDir(const char* pDir);

	MessageList *GetIdleMessageList();
	MessageList *GetLogMessageList();

public:
	MessageList mIdleMessageList;
	MessageList mLogMessageList;

private:
	KThread *mpLogThread;
	bool mIsRunning;
};

#endif /* LOGMANAGER_H_ */
