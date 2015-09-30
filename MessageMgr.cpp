
#include "MessageMgr.hpp"

DATA_MESSAGE_LIST g_IdleMsglist;                     //Idle List
pthread_mutex_t   g_tIdleMsgLock;                    //Idle Lock

const unsigned long g_iCount = 100000;
static unsigned long g_iInitCount = 1000;

//Msg list manager begin
MsgListManager::MsgListManager() {
    pthread_mutex_init(&m_tMessageLock, NULL);
    m_pOrgMsgList = NULL;
}

MsgListManager::~MsgListManager() {
    DestoryList();
    pthread_mutex_destroy(&m_tMessageLock);
}

void MsgListManager::Init(DATA_MESSAGE_LIST* pOrgMsgList) {
    if (pOrgMsgList) {
        m_pOrgMsgList = pOrgMsgList;
    }
}

int MsgListManager::GetMsgListBuffCount() {
    CAutoLock lock(&m_tMessageLock);
    return m_MsgList.size();
}

void MsgListManager::DestoryList() {
    LPMESSAGE_DATA pMsg = NULL;

    while (m_MsgList.size()) {
        pMsg = m_MsgList.front();
        m_MsgList.pop_front();
	    if (pMsg) {
            if (pMsg->pData) delete pMsg->pData;
            delete pMsg;
            pMsg = NULL;
        }
    }
    m_MsgList.clear();
}

//Read Msg
LPMESSAGE_DATA MsgListManager::Get_Msg() {
    LPMESSAGE_DATA pMsg = NULL;

    pthread_mutex_lock(&m_tMessageLock);
    if (m_MsgList.size() == 0) {
        pthread_mutex_unlock(&m_tMessageLock);
        g_eMessageEvent.wait();
	    pthread_mutex_lock(&m_tMessageLock);
    }

    if (m_MsgList.size() > 0) {
        pMsg = m_MsgList.front();
        m_MsgList.pop_front();
    }
    pthread_mutex_unlock(&m_tMessageLock);
    return pMsg;
}

//Add Msg
int MsgListManager::Put_Msg(LPMESSAGE_DATA apMsg) {
    CAutoLock lock(&m_tMessageLock);
    m_MsgList.push_back(apMsg);

    if (m_MsgList.size() == 1)
        g_eMessageEvent.signal();
    return 0;
}

//Restore Msg to original
void MsgListManager::ResMsg() {
    LPMESSAGE_DATA pMsg = NULL;

    CAutoLock lock(&m_tMessageLock);
    while (m_MsgList.size()) {
        pMsg = m_MsgList.front();
        m_MsgList.pop_front();
        if (m_pOrgMsgList) {
            m_pOrgMsgList->push_back(pMsg);
        }
    }
}
//Msg list manager end

//the follow function used in all of the application, so we use it in global
void InitMsgList(unsigned long nCount) {
    if (g_iCount < nCount) nCount = g_iCount;
    g_iInitCount = nCount;
    for (int i = 0; i < nCount; i++) {
        LPMESSAGE_DATA pMsg = new MESSAGE_DATA;
        if (pMsg) {
            pMsg->nType = OP_CLOSE;
            pMsg->nStatus = ST_STATIC;
            pMsg->pData = new DRVER_DATA();
            if (!pMsg->pData) continue;
            g_IdleMsglist.push_back(pMsg);
        }
    }
    pthread_mutex_init(&g_tIdleMsgLock, NULL);
}

void DestoryMsgList() {
    LPMESSAGE_DATA pMsg = NULL;

    while (g_IdleMsglist.size()) {
        pMsg = g_IdleMsglist.front();
        g_IdleMsglist.pop_front();
	    if (pMsg) {
            if (pMsg->pData) delete pMsg->pData;
            delete pMsg;
            pMsg = NULL;
        }
    }
    g_IdleMsglist.clear();

    pthread_mutex_destroy(&g_tIdleMsgLock);
}

//get buffer count
int GetIdleMsgBuffCount() {
    CAutoLock lock(&g_tIdleMsgLock);
    return g_IdleMsglist.size();
}

int GetMsgLimit() {
    return g_iCount;
}

int GetInitMsgLimit() {
    return g_iInitCount;
}

DATA_MESSAGE_LIST* GetIdleList() {
    return &g_IdleMsglist;
}

//Restort Idle Msg
int PutIdleMsgBuff(LPMESSAGE_DATA apMsg) {
    CAutoLock lock(&g_tIdleMsgLock);
    if (apMsg) {
        if (g_iCount < g_IdleMsglist.size()) {
            if (apMsg->nStatus == ST_DYNAMIC) {
                if (apMsg->pData) delete apMsg->pData;
                delete apMsg;
                //printf("MessageMgr : remove Dynamic buffer.");
                return 0;
            }
        }
        apMsg->nType = OP_CLOSE;
        if (apMsg->pData)
            apMsg->pData->ReSet();
        g_IdleMsglist.push_back(apMsg);
    }
    return 0;
}

//Get Idle Msg
LPMESSAGE_DATA GetIdleMsgBuff() {
    LPMESSAGE_DATA pMsg = NULL;

    CAutoLock lock(&g_tIdleMsgLock);
    if (g_IdleMsglist.size() > 0) {
        pMsg = g_IdleMsglist.front();
        g_IdleMsglist.pop_front();
        pMsg->reset();
    } else {
        //printf("MessageMgr : idle buffer Not enough, new again.");
        try {
            pMsg = new MESSAGE_DATA;
        } catch (...) {
            return NULL;
        }
        pMsg->nType = OP_CLOSE;
        pMsg->nStatus = ST_DYNAMIC;
        pMsg->sTimes = 0;
        try {
            pMsg->pData = new DRVER_DATA();
        } catch (...) {
            delete pMsg;
            pMsg = NULL;
        }
    }
    return pMsg;
}

