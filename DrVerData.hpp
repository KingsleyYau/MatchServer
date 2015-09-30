/*
 * File         : DrVerData.hpp
 * Date         : 2010-06-27
 * Author       : Keqin Su
 * Copyright    : City Hotspot Co., Ltd.
 * Description  : Class for drcom version server Data
 */

#ifndef _INC_DRVERDATA_
#define _INC_DRVERDATA_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include "LogFile.hpp"

using namespace std;

#ifndef BUFFER_SIZE_4
    #define BUFFER_SIZE_4       4
#endif

#ifndef BUFFER_SIZE_8
    #define BUFFER_SIZE_8       8
#endif

#ifndef BUFFER_SIZE_16
    #define BUFFER_SIZE_16      16
#endif

#ifndef BUFFER_SIZE_32
    #define BUFFER_SIZE_32      32
#endif

#ifndef BUFFER_SIZE_64
    #define BUFFER_SIZE_64      64
#endif

#ifndef BUFFER_SIZE_128
    #define BUFFER_SIZE_128     128
#endif

#ifndef BUFFER_SIZE_256
    #define BUFFER_SIZE_256     256
#endif

#ifndef BUFFER_SIZE_512
    #define BUFFER_SIZE_512     512
#endif

#ifndef BUFFER_SIZE_1K
    #define BUFFER_SIZE_1K      1024
#endif

#ifndef BUFFER_SIZE_2K
    #define BUFFER_SIZE_2K      2048
#endif

#ifndef BUFFER_SIZE_5K
    #define BUFFER_SIZE_5K      5120
#endif

#ifndef BUFFER_SIZE_10K
    #define BUFFER_SIZE_10K     10240
#endif

#ifndef true
    #define true                (bool)1
#endif

#ifndef false
    #define false               (bool)0
#endif

#ifndef __int64
    typedef long long __int64;
#endif

#ifndef MAX_SEND_TIMES
    #define MAX_SEND_TIMES      3
#endif

#ifndef MAX_SEND_TIME
    #define MAX_SEND_TIME       3000
#endif

#ifndef SIT_BACK
    #define SIT_BACK            10000
#endif

#ifndef GET_SOME_REST
    #define GET_SOME_REST       100000
#endif

#ifndef TIME_DIFFERENCE
    #define TIME_DIFFERENCE     3600
#endif

#ifndef MAX_CONNECTION_COUNT
    #define MAX_CONNECTION_COUNT    65535
#endif

//deal with change 404 to 201 (return 201 while version not be accpeted, it can reduce the update request)
#ifndef HTTP_NOVERISNEWEST
    #define HTTP_NOVERISNEWEST  605
#endif

#define END_WITH_LINE           "\r\n"
#define END_WITH_HTTP           "\r\n\r\n"

#define HTTP_200_OK             "200 OK"

#define HTTP_200                "200 NEED UPDATE"
#define HTTP_201                "201 LATEST VERSION"
#define HTTP_203                "203 OUT OF DATE"
#define HTTP_400                "400 BAD REQUEST"
#define HTTP_403                "403 FORBIDDEN"
#define HTTP_404                "404 NOT ACCEPT"
#define HTTP_404_TO_201         "201 NOT ACCEPT (404)"
#define HTTP_500                "500 REMOTE ERROR"

#define HTTP_KEEPALIVE          "KEEP-ALIVE"
#define HTTP_TEXT               "TEXT/PLAIN; CHARSET=UTF-8"
#define HTTP_ZIP                "APPLICATION/ZIP"
#define HTTP_CONTENTLENGTH      "CONTENT-LENGTH:"

#define HTTP_ELE_TIME           "TIME"
#define HTTP_ELE_TYPE           "TYPE"
#define HTTP_ELE_KEY            "KEY"
#define HTTP_ELE_HASH           "HASH"
#define HTTP_ELE_VER            "VER"
#define HTTP_ELE_RND            "RND"
#define HTTP_ELE_CHK            "CHK"

#define HTTP_TYPE_0000          "0000"
#define HTTP_TYPE_0003          "0003"
#define HTTP_TYPE_0006          "0006"

#define HTTP_GET                "GET "
#define HTTP_UPDATE             "/DRCLIENT/UPDATE?"
#define HTTP_RELOAD             "/DRCLIENT/RELOAD"
#define HTTP_CLOSE              "/DRCLIENT/CLOSE=7050169F1FC28F149A9EFCED2F6FB45C5076E43E" // SHA1(Dr.COM_PushServer)
//#define HTTP_REPONSE            "HTTP/1.0 %s\r\nSERVER: Dr.COM VER SERVER\r\nCONTENT-TYPE: %s\r\nCONTENT-LENGTH: %d\r\nCONECTION: CLOSE\r\n\r\n"
#define HTTP_REPONSE            "HTTP/1.0 %s\r\nSERVER: Dr.COM VER SERVER\r\nCONTENT-TYPE: %s\r\nCONTENT-LENGTH: %d\r\nCONECTION: KEEP-ALIVE\r\n\r\n"
#define HTTP_REPONSE_NOLENGTH   "HTTP/1.0 %s\r\nSERVER: Dr.COM VER SERVER\r\nCONTENT-TYPE: %s\r\n\r\n"
#define HTTP_REPONSE_CLOSE      "HTTP/1.0 %s\r\nSERVER: Dr.COM VER SERVER\r\nCONTENT-TYPE: %s\r\nCONTENT-LENGTH: %d\r\nCONECTION: CLOSE\r\n\r\n"

#define HTTP_ZIP_INDEX          "update.idx"

#define LEAK_100004             "1.0.0.201102011.A.W.100004"

static char* Http_GetRspByid(int id) {
    switch (id) {
        case 200:
            return HTTP_200;
        case 201:
            return HTTP_201;
        case 203:
            return HTTP_203;
        case 400:
            return HTTP_400;
        case 403:
            return HTTP_403;
        case 404:
            return HTTP_404;
        case HTTP_NOVERISNEWEST:
            return HTTP_404_TO_201;
        case 500:
        default:
            return HTTP_500;
    };
};

static void UpCase(char* pData, int iLen) {
    for (int i = 0; i <= iLen; i++){
        if (pData[i] >= 'a' && pData[i] <= 'z'){
            pData[i] = pData[i] - 32;
        }
    }
}

typedef struct st_BaseConf
{
    string m_strBindAdd;
    unsigned short m_usPort;
    unsigned int m_uiMaxConnect;
    unsigned short m_usProcPool;
    unsigned short m_usSndPool;
    unsigned short m_usRecvPool;
    unsigned short m_usWaitSndPool;

    st_BaseConf() {
        m_strBindAdd = "0.0.0.0";
        m_usPort = 80;
        m_uiMaxConnect = 102400;
        m_usProcPool = 20;
        m_usSndPool = 10;
        m_usRecvPool = 10;
        m_usWaitSndPool = 10;
    }
}BASE_CONF, *LPBASE_CONF;

typedef struct st_InsideConf
{
    string m_strBindAdd;
    unsigned short m_usPort;

    st_InsideConf() {
        m_strBindAdd = "192.168.0.1";
        m_usPort = 81;
    }
}INSIDE_CONF, *LPINSIDE_CONF;

typedef struct st_DBConf
{
    string m_strServer;
    string m_strUserName;
    string m_strPass;
    string m_strDbName;
    unsigned short m_usPort;
    unsigned short m_usThreadPool;

    st_DBConf() {
        m_strServer = "localhost";
        m_strUserName = "root";
        m_strPass = "admin";
        m_strDbName = "drcom";
        m_usPort = 3306;
        m_usThreadPool = 10;
    }
}DB_CONF, *LPDB_CONF;

typedef struct st_Leak
{
    bool m_b100004;

    st_Leak() {
        m_b100004 = false;
    }
}LEAK, *LPLEAK;

typedef struct st_SysConf
{
    //base config
    BASE_CONF m_BaseConf;
    //inside config
    INSIDE_CONF m_InsideConf;
    //db config
    DB_CONF m_DBConf;
    //for leak recover
    LEAK m_Leak;

    //use time verity
    bool bTimeChk;
    //return 201 while version not be accpeted, it can reduce the update request
    bool bNoVerIsNewest;
    //log path
    string strLogPath;
    //temp path
    string strTempPath;

    st_SysConf() {
        bTimeChk = true;
        bNoVerIsNewest = false;
        strLogPath = "log/";
        strTempPath = "temp/";
    };
}SYS_CONF, *LPSYS_CONF;

typedef struct st_DrVerData
{
    char m_cTime[BUFFER_SIZE_16];
    char m_cType[BUFFER_SIZE_8];
    char m_cKey[BUFFER_SIZE_256];
    char m_cHash[BUFFER_SIZE_256];
    char m_cVer[BUFFER_SIZE_128];
    char m_cRnd[BUFFER_SIZE_16];
    char m_cChk[BUFFER_SIZE_256];
    char m_cBuff256[BUFFER_SIZE_256];
    char m_cBuff16[BUFFER_SIZE_16];
    char m_cRsp[BUFFER_SIZE_10K];
    unsigned short m_usRspTotal;
    unsigned short m_usRspSend;

    //Compatible version
    bool m_bCompatible;

    //extern for accelerate, resolve from m_cVer
    __int64 m_i6Ver;
    bool m_bTest;
    char m_cUserPlatform[BUFFER_SIZE_64];

    st_DrVerData() {
        ReSet();
    }

    void ReSet() {
        memset(m_cTime, 0, sizeof(m_cTime));
        memset(m_cType, 0, sizeof(m_cType));
        memset(m_cKey, 0, sizeof(m_cKey));
        memset(m_cHash, 0, sizeof(m_cHash));
        memset(m_cVer, 0, sizeof(m_cVer));
        memset(m_cRnd, 0, sizeof(m_cRnd));
        memset(m_cChk, 0, sizeof(m_cChk));
        memset(m_cRsp, 0, sizeof(m_cRsp));
        m_usRspTotal = 0;
        m_usRspSend = 0;

        m_bCompatible = true;

        exReSet();
    }

    void InitBySelf(const st_DrVerData* pData) {
        memcpy(m_cTime, pData->m_cTime, sizeof(m_cTime));
        memcpy(m_cType, pData->m_cType, sizeof(m_cType));
        memcpy(m_cKey, pData->m_cKey, sizeof(m_cKey));
        memcpy(m_cHash, pData->m_cHash, sizeof(m_cHash));
        memcpy(m_cVer, pData->m_cVer, sizeof(m_cVer));
        memcpy(m_cRnd, pData->m_cRnd, sizeof(m_cRnd));
        memcpy(m_cChk, pData->m_cChk, sizeof(m_cChk));
        memcpy(m_cRsp, pData->m_cRsp, sizeof(m_cRsp));
        m_usRspTotal = pData->m_usRspTotal;
        m_usRspSend = pData->m_usRspSend;

        m_bCompatible = pData->m_bCompatible;

        m_i6Ver = pData->m_i6Ver;
        m_bTest = pData->m_bTest;
        memcpy(m_cUserPlatform, pData->m_cUserPlatform, sizeof(m_cUserPlatform));
    }

    inline void exReSet() {
        m_i6Ver = 0LL;
        m_bTest = false;
        memset(m_cUserPlatform, 0, sizeof(m_cUserPlatform));
    }

    bool TransVer(const string str) {
        exReSet();

        sprintf(m_cUserPlatform, "%s", str.c_str());
        char* token = strtok(m_cUserPlatform, ".");
        int i = 0, j = 0, k = 0;
        bool bFlag = false;
        while (token != NULL) {
            if ((strlen(token) > 3 && i < 3) || (strlen(token) != 9 &&  i == 3) ||
                ((token[0] != 'W') && (token[0] != 'M') && (token[0] != 'L') &&  i == 5)) {
                exReSet();
                break;
            }
            if (i <= 3) {
                j += strlen(token) + 1;
            }
            if (i == 0) {
                m_i6Ver = (__int64)atoi(token) * (__int64)pow(10, 14);
            } else if (i == 1) {
                m_i6Ver += (__int64)atoi(token) * (__int64)pow(10, 11);
            } else if (i == 2) {
                m_i6Ver += (__int64)atoi(token) * (__int64)pow(10, 8);
            } else if (i == 3) {
                k = atoi(token);
                if ((k % 10) == 2){
                    m_bTest = true;
                }
                m_i6Ver += (__int64)k / 10;
            } else if (i == 5) {
                bFlag = true;
                break;
            }
            i++;
            token = strtok(NULL, ".");
        }
        if (bFlag) {
            sprintf(m_cUserPlatform, "%s", str.substr(j, str.length()).c_str());
        }
        return bFlag;

    }
}DRVER_DATA, *LPDRVER_DATA;

#endif
