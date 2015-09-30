
#ifndef _INC_MESSAGEMGR_
#define _INC_MESSAGEMGR_

#include <list>
#include <map>
#include "event.hpp"
#include "DrVerData.hpp"

enum MESSAGETYPE{
    OP_CLOSE = 0,
    OP_RECV ,
    OP_SEND,
};

enum STATUSTYPE{
    ST_STATIC = 0,
    ST_DYNAMIC,
};

typedef struct st_MessgaeData
{
    MESSAGETYPE     nType;                                      //Msg Type
    STATUSTYPE      nStatus;
    int             iFd;
    LPDRVER_DATA    pData;
    unsigned short  sTimes;                                     //repeat times
    pthread_mutex_t tMutex;                                     //lock
    unsigned int    nContextLength;
    unsigned int    nHeaderLength;
    bool            bSendAndClose;
    char            cBuffer[64];
    long            lLastSendTime;
    bool            bDBData;

    st_MessgaeData() {
//        nType = OP_CLOSE;
//        nStatus = ST_STATIC;
//        iFd = -1;
//        pData = NULL;
//        sTimes = 0;
//        nContextLength = 0;
//        nHeaderLength = 0;
//        bSendAndClose = false;
//        lLastSendTime = 0;
//        bDBData = false;
//        memset(cBuffer, 0, sizeof(cBuffer));
        reset();
        pthread_mutex_init(&tMutex, NULL);
    };

    ~st_MessgaeData() {
        pthread_mutex_destroy(&tMutex);
    }

    void reset(){
        nType = OP_CLOSE;
        nStatus = ST_STATIC;
        iFd = -1;
        sTimes = 0;
        nContextLength = 0;
        nHeaderLength = 0;
        bSendAndClose = false;
        lLastSendTime = 0;
        bDBData = false;
        memset(cBuffer, 0, sizeof(cBuffer));
    }
}MESSAGE_DATA, *LPMESSAGE_DATA;

typedef list < LPMESSAGE_DATA > DATA_MESSAGE_LIST;              //Msg list

#ifdef __cplusplus
extern "C" {
#endif

void InitMsgList(unsigned long nCount);
void DestoryMsgList();

//get buffer count
int GetIdleMsgBuffCount();
int GetMsgLimit();
int GetInitMsgLimit();
DATA_MESSAGE_LIST* GetIdleList();
//Restort Idle Msg
int PutIdleMsgBuff(LPMESSAGE_DATA apMsg);
//Get Idle Msg
LPMESSAGE_DATA GetIdleMsgBuff();


class MsgListManager
{
    public:
        MsgListManager();
        ~MsgListManager();

        void Init(DATA_MESSAGE_LIST* pOrgMsgList);
        int GetMsgListBuffCount();
        void DestoryList();
        //Read Msg
        LPMESSAGE_DATA Get_Msg();
        //Add Msg
        int Put_Msg(LPMESSAGE_DATA apMsg);

        //Restore Msg to original
        void ResMsg();

    protected:
        DATA_MESSAGE_LIST  m_MsgList;                      //Data List
        DATA_MESSAGE_LIST* m_pOrgMsgList;                  //Original Data List
        pthread_mutex_t    m_tMessageLock;                 //Data Lock
        Event              g_eMessageEvent;                //Data event
};

#ifdef __cplusplus
}
#endif
#endif
