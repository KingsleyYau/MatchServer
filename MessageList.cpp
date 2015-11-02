/*
 * MessageList.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "MessageList.h"

MessageList::MessageList() {
	// TODO Auto-generated constructor stub
	pthread_rwlock_init(&m_Lock, NULL);
	m_Size = 0;
}

MessageList::~MessageList() {
	// TODO Auto-generated destructor stub
	pthread_rwlock_destroy(&m_Lock);
}
void MessageList::PushBack(Message *m) {
	pthread_rwlock_wrlock(&m_Lock);
	m_List.push_back(m);
	m_Size++;
	pthread_rwlock_unlock(&m_Lock);
}
Message* MessageList::PopFront() {
	Message *m = NULL;
	pthread_rwlock_wrlock(&m_Lock);
	if( !m_List.empty() ) {
		m = m_List.front();
		m_List.pop_front();
		m_Size--;
	}
	pthread_rwlock_unlock(&m_Lock);
	return m;
}
bool MessageList::Empty() {
	bool bFlag = false;
	pthread_rwlock_rdlock(&m_Lock);
	bFlag = m_List.empty();
	pthread_rwlock_unlock(&m_Lock);
	return bFlag;
}
size_t MessageList::Size() {
	size_t size = 0;
	pthread_rwlock_rdlock(&m_Lock);
	size = m_Size;
	pthread_rwlock_unlock(&m_Lock);
	return m_Size;
}
