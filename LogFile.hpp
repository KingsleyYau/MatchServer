
#ifndef __FILECTRL_H__
#define __FILECTRL_H__

#include <pthread.h>
#include <stdio.h>

typedef unsigned long   LOG_LEVEL;

#define LOG_STAT        (LOG_LEVEL)5
#define LOG_MSG         (LOG_LEVEL)4
#define LOG_WARNING     (LOG_LEVEL)3
#define LOG_ERR_USER    (LOG_LEVEL)2
#define LOG_ERR_SYS     (LOG_LEVEL)1   	
#define LOG_OFF         (LOG_LEVEL)0

#ifndef MAX_PATH
    #define MAX_PATH        256
#endif
#ifndef BUFFER_SIZE_1K
    #define BUFFER_SIZE_1K  1024
#endif
#ifndef BUFFER_SIZE_2K
    #define BUFFER_SIZE_2K  2048
#endif
#ifndef BUFFER_SIZE_5K
    #define BUFFER_SIZE_5K  5120
#endif

extern unsigned long    g_iLogLevel;

/*
  create a memory block, flush memory into file by application or memories were filled [ usLen == 0 then flush into file immediately]
  make sure the memory block is larger than 1 record
 */

void SetFlushBuffer(unsigned int iLen);
void FlushMem2File();

//#define gvLog saveLog

//int saveLog(LOG_LEVEL nLevel, const char *szfmt, ...);

class CFileCtrl
{
public:
    CFileCtrl(const char *szlogPath, const char* szLogFileName, unsigned long alFileLen = 6, int bSingle = 0);
    virtual ~CFileCtrl();

public:
    int Initialize();
    FILE* OpenLogFile();
    FILE* FileOpen(const char *szFileName, int aiMod);
    int LogMsg(const char* pszFormat, int aiLen, const char* pszHead);
    int ReadMsg(char *szFormat, int aiLen);
    bool SetFileSeek(unsigned long nOffset);
    int printLog(const char *szfmt, ...);
    void Mem2File();
	
    bool IsEnd();

    int CloseFile();
	
    long GetFileLen() {return m_nFileLen;};
    long GetCurLen() {return m_ncurRead;};

protected:
    FILE* CreateLog();

private:
    pthread_mutex_t m_hMutex;
    FILE* m_pLogFile;
    unsigned long m_dwMaxFileLength;    //Max file length
    char m_szLogFileName[MAX_PATH];     //File name
    char m_szLogPath[MAX_PATH];         //Path name
    long m_nFileLen;                    //file size
    long m_ncurRead;                    //current position
    int m_bSingle;                      //need for mutithread
    char* m_pBuffer;                    //buffer
    long m_nBufferUse;                  //buffer in use
};
extern CFileCtrl *g_pFileCtrl;              //Log class
#endif
