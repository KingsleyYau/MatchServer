/*
 * MessageList.h
 *
 *  Created on: 2015-9-28
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef MESSAGELIST_H_
#define MESSAGELIST_H_

#include <common/Buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ev.h>

#include <pthread.h>

#include <list>
using namespace std;

typedef enum MessageType {
	MessageTypeRecv,
	MessageTypeSend
};

typedef struct Message {
	int		fd;
	int 	len;
	int		index;
	int		seq;

	MessageType		type;

	unsigned int 	starttime;

	char	buffer[MAX_LOG_BUFFER_LEN];
//	char    bitBuffer[128];
	ev_io *wr;
	ev_io *ww;

	void Reset() {
		len = 0;
		seq = 0;
		type = MessageTypeRecv;
		starttime = 0;
		memset(buffer, '\0', sizeof(buffer));
//		memset(bitBuffer, '\0', sizeof(bitBuffer));
		wr = NULL;
		ww = NULL;
	}
} Message;

class MessageList {
public:
	MessageList();
	virtual ~MessageList();

	void PushBack(Message *m);
	Message* PopFront();
	bool Empty();
	size_t Size();

	list<Message *> m_List;
	pthread_rwlock_t m_Lock;
	size_t m_Size;
};

#endif /* MESSAGELIST_H_ */
