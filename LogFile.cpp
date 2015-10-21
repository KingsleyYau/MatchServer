
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "LogFile.hpp"

LOG_LEVEL g_iLogLevel = LOG_MSG;
CFileCtrl *g_pFileCtrl = NULL;              //Log class
//static int g_bCreated = false;              //create flag
static unsigned g_iMemSize = 0;
pthread_mutex_t m_hMutexBuffer;

void SetFlushBuffer(unsigned int iLen)
{
    g_iMemSize = iLen;
}

void FlushMem2File()
{
    if (g_pFileCtrl) {
        pthread_mutex_lock(&m_hMutexBuffer);
        g_pFileCtrl->Mem2File();
        pthread_mutex_unlock(&m_hMutexBuffer);
    }
}

//int saveLog(LOG_LEVEL nLevel, const char *szfmt, ...)
//{
//    if (g_iLogLevel >= nLevel) {
//        if (g_bCreated == false) {
//            g_bCreated = true;
//            if (g_pFileCtrl) {
//                delete g_pFileCtrl;
//            }
//            g_pFileCtrl = NULL;
//            g_pFileCtrl = new CFileCtrl(g_SysConf.strLogPath.c_str(), "Log", 128);
//            if (!g_pFileCtrl) {
//                return -1;
//            }
//            g_pFileCtrl->Initialize();
//            g_pFileCtrl->OpenLogFile();
//	    }
//	    LPMESSAGE_DATA pMsg = GetIdleMsgBuff();
//	    pMsg->pData->ReSet();
//
//	    //get current time
//	    time_t stm = time(NULL);
//        struct tm tTime;
//        localtime_r(&stm,&tTime);
//        snprintf(pMsg->pData->m_cChk, 64, "[ %d-%02d-%02d %02d:%02d:%02d ] ", tTime.tm_year+1900, tTime.tm_mon+1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);
//
//        //get va_list
//        char *p = pMsg->pData->m_cRsp;
//        va_list	agList;
//        va_start(agList, szfmt);
//        vsnprintf(p, BUFFER_SIZE_5K, szfmt, agList);
//        va_end(agList);
//
//        strcat(pMsg->pData->m_cRsp, "\n");
//        g_pFileCtrl->LogMsg(pMsg->pData->m_cRsp, (int)strlen(pMsg->pData->m_cRsp), pMsg->pData->m_cChk);
//
//        PutIdleMsgBuff(pMsg);
//        return 1;
//    }
//    return 0;
//}

CFileCtrl::CFileCtrl(const char *szlogPath, const char* szFileName, unsigned long aiFileLen, int bSingle)
{
    m_dwMaxFileLength = aiFileLen;
    m_pLogFile = NULL;
    memset(m_szLogFileName, 0, MAX_PATH * sizeof(char));
    snprintf(m_szLogFileName, MAX_PATH, "%s", szFileName);
    
    memset(m_szLogPath, 0, MAX_PATH * sizeof(char));
    snprintf(m_szLogPath, MAX_PATH, "%s", szlogPath);
    
    //printf("%s:%s", m_szLogPath, m_szLogFileName );
	m_bSingle = bSingle;
	m_pBuffer = NULL;
	m_nBufferUse = 0;
	if (g_iMemSize) {
	    m_pBuffer = new char[g_iMemSize];
	}
}

CFileCtrl::~CFileCtrl()
{
    pthread_mutex_destroy(&m_hMutex);
    pthread_mutex_destroy(&m_hMutexBuffer);
    
    CloseFile();
    if (m_pBuffer) {
        delete m_pBuffer;
    }
}

int CFileCtrl::CloseFile()
{
    if (m_pLogFile){
        fclose(m_pLogFile);
        m_pLogFile = NULL;
    }
    return 0;
}

int CFileCtrl::Initialize()
{
    if (m_dwMaxFileLength == 0) {
        m_dwMaxFileLength = 2;
    }

    m_dwMaxFileLength = m_dwMaxFileLength * BUFFER_SIZE_1K * BUFFER_SIZE_1K;
    pthread_mutex_init(&m_hMutex, NULL);
    pthread_mutex_init(&m_hMutexBuffer, NULL);
	return 0;
}

FILE*  CFileCtrl::OpenLogFile()
{
    return CreateLog();
}

FILE* CFileCtrl::CreateLog()
{	
    char szTime[MAX_PATH];
    memset(szTime,0,MAX_PATH);
    
	time_t stm=time(NULL);
	struct tm tTime;
	localtime_r(&stm,&tTime);
	
	DIR *dp=opendir(m_szLogPath);
	if (dp==NULL) {
		int mod=S_IREAD | S_IWRITE| S_IEXEC | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ;
		mkdir(m_szLogPath, mod);
	} else {
		closedir(dp);
	}
    snprintf(szTime, MAX_PATH, "%s/%s%d%02d%02d%02d%02d%02d.txt", m_szLogPath, m_szLogFileName, tTime.tm_year+1900, tTime.tm_mon+1, tTime.tm_mday, tTime.tm_hour, tTime.tm_min, tTime.tm_sec);

    if (m_pLogFile) {
        fclose(m_pLogFile);
        m_pLogFile = NULL;
        m_nFileLen = 0;
    }
    if ((m_pLogFile = fopen(szTime, "w+b")) == 0) {
        m_nFileLen = fseek(m_pLogFile, 0L, SEEK_END);
    }
    return m_pLogFile;
}

FILE*  CFileCtrl::FileOpen(const char *szFileName, int aiMod)
{
    if (m_pLogFile) {
        fclose(m_pLogFile);
        m_pLogFile = NULL;
    }

    m_nFileLen = 0;
    m_ncurRead = 0;

    if ((m_pLogFile = fopen(szFileName, "w+b")) == 0) {
        m_nFileLen = fseek(m_pLogFile, 0L, SEEK_END);
        fseek(m_pLogFile, 0 - m_nFileLen, SEEK_CUR);
    }
    return m_pLogFile;
}

bool CFileCtrl::SetFileSeek(unsigned long  nOffset)
{	
    return true;
}

int CFileCtrl::LogMsg(const char* pszFormat, int aiLen, const char* pszHead)
{
    if (m_pLogFile == NULL || !pszFormat) {
        return -1;
    }

    if (!m_bSingle) {
        pthread_mutex_lock(&m_hMutex); 
    }
    if (!m_pBuffer) {
        int len = (int)fwrite(pszHead, sizeof(char), (unsigned int)strlen(pszHead), m_pLogFile);
        m_nFileLen += len;
        len = (int)fwrite(pszFormat, sizeof(char), (unsigned int)aiLen, m_pLogFile);
    	//file size
        m_nFileLen += len;
        
        fflush(m_pLogFile);
    } else {
        pthread_mutex_lock(&m_hMutexBuffer); 
        
        if ((m_nBufferUse + (unsigned int)strlen(pszHead) + (unsigned int)aiLen) >= g_iMemSize) {//memory full
            Mem2File();//change m_nBufferUse to 0
        }
        //add buffer len for those in use
        memcpy(m_pBuffer + m_nBufferUse, pszHead, (unsigned int)strlen(pszHead));
        m_nBufferUse += (unsigned int)strlen(pszHead);
        memcpy(m_pBuffer + m_nBufferUse, pszFormat, (unsigned int)aiLen);
        m_nBufferUse += (unsigned int)aiLen;
        
        pthread_mutex_unlock(&m_hMutexBuffer);
        
        m_nFileLen = m_nFileLen +  (unsigned int)strlen(pszHead) + (unsigned int)aiLen;//add file len for create file
    }

	//over file size, create again
    if (m_nFileLen >= (long)m_dwMaxFileLength) {
        Mem2File();
        CreateLog();
    }
    if (!m_bSingle){
        pthread_mutex_unlock(&m_hMutex);
    }
    
    return 0;
}

int CFileCtrl::ReadMsg(char *szFormat, int aiLen)
{	
    if (!szFormat || m_pLogFile == NULL) {
        return -1;
    }
	
    if (!m_bSingle) {
		pthread_mutex_lock(&m_hMutex); 
    }
	
    int len = (int)fread(szFormat, sizeof(char), aiLen, m_pLogFile);

	//current size
    m_ncurRead += len;
	//over file size, create again
    if (m_ncurRead >= m_nFileLen) {
        fclose(m_pLogFile);
        m_pLogFile=NULL;
		
        if (!m_bSingle) {
            pthread_mutex_unlock(&m_hMutex); 
        }
        return len;
    }
    fseek(m_pLogFile, len, SEEK_CUR);

    if (!m_bSingle) {
        pthread_mutex_unlock(&m_hMutex); 
    }
    return len;
}

bool CFileCtrl::IsEnd()
{
    if (m_pLogFile == NULL) {
        return true;
    }

    if (m_ncurRead < m_nFileLen) {
        return false;
    }
    return true;	
}

int CFileCtrl::printLog(const char *szfmt, ...)
{	
    char lsbuf[BUFFER_SIZE_2K];
    memset(lsbuf, 0, BUFFER_SIZE_2K * sizeof(char));

    va_list args;
    va_start(args, szfmt);

    int nStrLen = (int)strlen(szfmt);
    if (nStrLen >= BUFFER_SIZE_2K) {
        return -1;
    }
    vsprintf(lsbuf, szfmt, args);
    va_end(args);

    //write file
    if (!m_bSingle) {
        pthread_mutex_lock(&m_hMutex); 
    }
    int len = (int)fwrite(lsbuf, sizeof(char), (unsigned int)strlen(lsbuf), m_pLogFile);
    m_nFileLen += len;
	
	//over file size, create again
    if (m_nFileLen >= (long)m_dwMaxFileLength) {
        CreateLog();
    }
    if (!m_bSingle) {
        pthread_mutex_unlock(&m_hMutex); 
    }
    return 0;
}

void CFileCtrl::Mem2File()
{
    if (m_pLogFile && m_pBuffer && (m_nBufferUse > 0)){
        (int)fwrite(m_pBuffer, sizeof(char), m_nBufferUse, m_pLogFile);
        m_nBufferUse = 0;
        fflush(m_pLogFile);
    }
}
