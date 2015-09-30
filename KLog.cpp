/*
 * DrLog.h
 *
 *  Created on: 2014/10/27
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */
#include "KLog.h"
#include <sys/syscall.h>

// modify by samson, 把定义放到头文件，给外部知道
//#define MAX_LOG_BUFFER 512 * 1024

static bool gLogEnable = true;
void KLog::SetLogEnable(bool enable) {
	gLogEnable = enable;
}

static string gLogDirectory = "/";
void KLog::SetLogDirectory(string directory) {
	gLogDirectory = directory;
}

int KLog::LogToFile(const char *fileNamePre, const char *logDir, const char *fmt, ...) {
	if( !gLogEnable ) {
		return 0;
	}

	char pTimeBuffer[1024] = {'\0'}, pDataBuffer[MAX_LOG_BUFFER] = {'\0'};

	timeval tNow;
	gettimeofday(&tNow, NULL);
	time_t stm = time(NULL);
	struct tm tTime;
	localtime_r(&stm, &tTime);
	int tid = (int)syscall(SYS_gettid);
	snprintf(pTimeBuffer, 64, "[ tid: %d, %d-%02d-%02d %02d:%02d:%02d.%03d ]",
			tid, tTime.tm_year + 1900, tTime.tm_mon + 1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec, tNow.tv_usec/1000);

    va_list	agList;
    va_start(agList, fmt);
    vsnprintf(pDataBuffer, sizeof(pDataBuffer)-1, fmt, agList);
    va_end(agList);

    strcat(pDataBuffer, "\n");

    string sLogFileDir = gLogDirectory;
    if( logDir != NULL && strlen(logDir) != 0 ) {
    	sLogFileDir = logDir;
    }
    string sLogFileName = fileNamePre;

    DIR* dir = opendir(sLogFileDir.c_str());
	if (dir == NULL) {
		int mod = S_IRWXU | S_IRWXG | S_IRWXO;
		if( 0 == mkdir(sLogFileDir.c_str(), mod) ) {

		}
	} else {
		closedir(dir);
	}

#ifdef PRINT_JNI_LOG
	DLog(fileNamePre, "%s", pDataBuffer);
#endif

    char pLogFilePath[1024] = {'\0'};
    snprintf(pLogFilePath, sizeof(pLogFilePath), "%s/%s-%d-%02d-%02d.log", \
    		sLogFileDir.c_str(), sLogFileName.c_str(), tTime.tm_year + 1900, tTime.tm_mon + 1, \
    		tTime.tm_mday/*, tTime.tm_hour, tTime.tm_min, tTime.tm_sec*/);

    FILE* file = fopen(pLogFilePath, "a+b");
    if(file != NULL) {

    	int iLen = -1;
    	fseek(file, 0L, SEEK_END);

    	if(NULL != pTimeBuffer) {
    		iLen = fwrite(pTimeBuffer, 1, strlen(pTimeBuffer), file);
    		if(iLen != -1 ) {

    		}
    	}
    	if(NULL != pDataBuffer) {
    		iLen = fwrite(pDataBuffer, 1, strlen(pDataBuffer), file);
    		if(iLen != -1 ) {

    		}
    	}
    	fclose(file);
    }
    else {
    }

	return 1;
}
