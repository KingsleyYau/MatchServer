/*
 * KMutex.h
 *
 *  Created on: 2014/10/27
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef _INC_KMutex_
#define _INC_KMutex_

#include <pthread.h>
using namespace std;

class KMutex
{
public:
	KMutex(){
		initlock();
	}
	~KMutex(){
		desrtoylock();
	}
	int trylock(){
		return pthread_mutex_trylock(&m_Mutex);
	}
	int lock(){
		return pthread_mutex_lock(&m_Mutex);
	}
	int unlock(){
		return pthread_mutex_unlock(&m_Mutex);
	}
protected:
	int initlock(){
		return pthread_mutex_init(&m_Mutex, NULL);
	}
	int desrtoylock(){
		return pthread_mutex_destroy(&m_Mutex);
	}
private:
	pthread_mutex_t m_Mutex;
	pthread_mutexattr_t m_Mutexattr;
};
#endif
