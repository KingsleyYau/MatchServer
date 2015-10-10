/*
 * KSafeList.h
 *
 *  Created on: 2015-3-2
 *      Author: Max.chiu
 */

#ifndef KSAFELIST_H_
#define KSAFELIST_H_

#include "KMutex.h"

#include <list>
using namespace std;

template<typename Value>
class KSafeList {
	typedef list<Value> SafeList;

public:
	KSafeList() {
		pthread_rwlock_init(&mLock, NULL);
		mSize = 0;
	}

	virtual ~KSafeList() {
		pthread_rwlock_destroy(&mLock);
	}

	void PushBack(Value v) {
		pthread_rwlock_wrlock(&mLock);
		mList.push_back(v);
		mSize++;
		pthread_rwlock_unlock(&mLock);
	}

	Value PopFront() {
		Value v = NULL;
		pthread_rwlock_wrlock(&mLock);
		if( !mList.empty() ) {
			v = mList.front();
			mList.pop_front();
			mSize--;
		}
		pthread_rwlock_unlock(&mLock);
		return v;
	}

	bool Empty() {
		bool bFlag = false;
		pthread_rwlock_rdlock(&mLock);
		bFlag = mList.empty();
		pthread_rwlock_unlock(&mLock);
		return bFlag;
	}

	size_t Size() {
		size_t size = 0;
		pthread_rwlock_rdlock(&mLock);
		size = mSize;
		pthread_rwlock_unlock(&mLock);
		return mSize;
	}

private:
	SafeList mList;
	pthread_rwlock_t mLock;
	size_t mSize;
};
#endif /* KSAFELIST_H_ */
