/*
 * KSafeMap.h
 *
 *  Created on: 2015-3-2
 *      Author: Max.chiu
 */

#ifndef KSafeMap_H_
#define KSafeMap_H_

#include "KMutex.h"

#include <map>
using namespace std;

template<typename Key, typename Value>
class KSafeMap {
	typedef map<Key, Value> SafeMap;

public:
	KSafeMap() {

	}
	virtual ~KSafeMap() {

	}

	void Insert(Key key, Value value) {
		mKMutex.lock();
		Value findValue = Find(key);
		if( findValue == 0 ) {
			mMap.insert( typename SafeMap::value_type(key, value) );
		}
		else {
			findValue = value;
		}
		mKMutex.unlock();
	}
	Value Erase(Key key) {
		Value value = 0;
		mKMutex.lock();
		typename SafeMap::iterator itr = mMap.find(key);
		if( itr != mMap.end() ) {
			mMap.erase(itr);
			value = itr->second;
		}
		mKMutex.unlock();
		return value;
	}
	void Empty() {
		mKMutex.lock();
		mMap.empty();
		mKMutex.unlock();
	}
private:
	Value Find(Key key) {
		Value value = 0;
		typename SafeMap::iterator itr = mMap.find(key);
		if( itr != mMap.end() ) {
			value = itr->second;
		}
		return value;
	}

	KMutex mKMutex;
	SafeMap mMap;
};
#endif /* KSafeMap_H_ */
