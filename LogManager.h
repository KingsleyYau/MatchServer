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
#include <unistd.h>
#include <stdarg.h>
#include <sys/syscall.h>

#include "MessageList.h"
#include "KThread.h"
#include "LogFile.hpp"

#include <string>
using namespace std;

#define DiffGetTickCount(start, end)    ((start) <= (end) ? (end) - (start) : ((unsigned int)(-1)) - (start) + (end))

class LogManager {
public:

	static LogManager *GetLogManager();

	static void LogSetFlushBuffer(unsigned int iLen);
	static void LogFlushMem2File();

	LogManager();
	virtual ~LogManager();

	bool Start(int maxIdle, LOG_LEVEL nLevel = LOG_STAT, const string& dir = "log");
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

	string mLogDir;
};

#endif /* LOGMANAGER_H_ */
