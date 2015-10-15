/*
 * LogManager.cpp
 *
 *  Created on: 2015-1-13
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "LogManager.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

// Just 4 log
static int g_bCreated = false;              //create flag

void LogManager::LogSetFlushBuffer(unsigned int iLen) {
    SetFlushBuffer(iLen);
}

void LogManager::LogFlushMem2File() {
	FlushMem2File();
}

/* log thread */
class LogRunnable : public KRunnable {
public:
	LogRunnable(LogManager *container) {
		mContainer = container;
	}
	virtual ~LogRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		int iCount = 0;
		while( mContainer->IsRunning() ) {
			if ( iCount < 5 ) {
				iCount++;
			} else {
				iCount = 0;
				LogManager::LogFlushMem2File();
			}
			sleep(1);
		}

//		while( mContainer->IsRunning() ) {
//			Message *m = NULL;
//			do {
//				m = mContainer->GetLogMessageList()->PopFront();
//				if( m == NULL ) {
//					break;
//				}
//
//				/* log data here */
//				char *buffer = m->buffer;
//
////				KLog::LogToFile("log", "", "%s", buffer);
//
////				printf("%s\n", buffer);
//
//				mContainer->GetIdleMessageList()->PushBack(m);
//			} while( NULL != m );
//			sleep(1);
//		}
	}

private:
	LogManager *mContainer;
};

static LogManager *gLogManager = NULL;
LogManager *LogManager::GetLogManager() {
	if( gLogManager == NULL ) {
		gLogManager = new LogManager();
	}
	return gLogManager;
}

LogManager::LogManager() {
	// TODO Auto-generated constructor stub
	mIsRunning = false;
}

LogManager::~LogManager() {
	// TODO Auto-generated destructor stub
	mIsRunning = false;
}

bool LogManager::Log(LOG_LEVEL nLevel, const char *format, ...) {
	if( !mIsRunning )
		return false;
//	if (g_iLogLevel >= nLevel) {
//		Message *lm = mIdleMessageList.PopFront();
//		if( lm != NULL ) {
//			lm->Reset();
//			va_list	agList;
//			va_start(agList, format);
//			vsnprintf(lm->buffer, MAXLEN - 1, format, agList);
//			va_end(agList);
//			mLogMessageList.PushBack(lm);
//			return true;
//		} else {
//			return false;
//		}
//	}

    if (g_iLogLevel >= nLevel) {
        if (g_bCreated == false) {
            g_bCreated = true;
            if (g_pFileCtrl) {
                delete g_pFileCtrl;
            }
            g_pFileCtrl = NULL;
            g_pFileCtrl = new CFileCtrl(mLogDir.c_str(), "Log", 128);
            if (!g_pFileCtrl) {
                return -1;
            }
            g_pFileCtrl->Initialize();
            g_pFileCtrl->OpenLogFile();
	    }

        Message *lm = mIdleMessageList.PopFront();
        if( lm != NULL ) {
            lm->Reset();

    	    //get current time
    	    time_t stm = time(NULL);
            struct tm tTime;
            localtime_r(&stm,&tTime);
            snprintf(lm->bitBuffer, 64, "[ %d-%02d-%02d %02d:%02d:%02d ] ", tTime.tm_year+1900, tTime.tm_mon+1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

            //get va_list
            char *p = lm->buffer;
            va_list	agList;
            va_start(agList, format);
            vsnprintf(p, MAXLEN, format, agList);
            va_end(agList);

            strcat(lm->buffer, "\n");
            g_pFileCtrl->LogMsg(lm->buffer, (int)strlen(lm->buffer), lm->bitBuffer);

            mIdleMessageList.PushBack(lm);
        }

        return true;
    }

	return true;
}

bool LogManager::Start(int maxIdle, LOG_LEVEL nLevel, const string& dir) {
	/* create log buffers */
	for(int i = 0; i < maxIdle; i++) {
		Message *m = new Message();
		mIdleMessageList.PushBack(m);
	}

	mIsRunning = true;

	// Just 4 log
	g_iLogLevel = nLevel;
    MkDir(dir.c_str());
    mLogDir = dir;
//    MkDir(g_SysConf.strTempPath.c_str());
//    InitMsgList(maxIdle);
//    KLog::SetLogDirectory(g_SysConf.strLogPath.c_str());

	/* start log thread */
	mpLogThread = new KThread(new LogRunnable(this));
	if( mpLogThread->start() != 0 ) {
//		printf("# Create log thread ok \n");
	}
	printf("# LogManager start OK. \n");
	return true;
}

bool LogManager::Stop() {
	/* stop log thread */
	mIsRunning = false;
	if( mpLogThread ) {
		mpLogThread->stop();
	}

	/* release log buffers */
	Message *m;

//	while( NULL != ( m = mLogMessageList.PopFront() ) ) {
//		char *buffer = m->buffer;
//		KLog::LogToFile("log", "", "%s \n", buffer);
//		delete m;
//	}

	while( NULL != ( m = mIdleMessageList.PopFront() ) ) {
		delete m;
	}

	return true;
}

bool LogManager::IsRunning() {
	return mIsRunning;
}

MessageList *LogManager::GetIdleMessageList() {
	return &mIdleMessageList;
}

MessageList *LogManager::GetLogMessageList() {
	return &mLogMessageList;
}

int LogManager::MkDir(const char* pDir) {
    int ret = 0;
    struct stat dirBuf;
    char cDir[BUFFER_SIZE_1K] = {0};
    char cTempDir[BUFFER_SIZE_1K] = {0};

    strcpy(cDir, pDir);
    if (pDir[strlen(pDir)-1] != '/') {
        cDir[strlen(pDir)] = '/';
    }
    int iLen = strlen(cDir);
    for (int i = 0; i < iLen; i++) {
        if ('/' == cDir[i]) {
            if (0 == i) {
                strncpy(cTempDir, cDir, i+1);
                cTempDir[i+1] = '\0';
            } else {
                strncpy(cTempDir, cDir, i);
                cTempDir[i] = '\0';
            }
            ret = stat(cTempDir, &dirBuf);
            if (-1 == ret &&  ENOENT == errno) {
                ret = mkdir(cTempDir, S_IRWXU | S_IRWXG | S_IRWXO);
                if (-1 == ret) {
                    return 0;
                }
                chmod(cTempDir, S_IRWXU | S_IRWXG | S_IRWXO);
            } else if (-1 == ret) {
                return 0;
            }
            if (!(S_IFDIR & dirBuf.st_mode)) {
                return 0;
            }
        }
    }
    return 1;
}
