/*
 * MatchServerTest.cpp
 *
 *  Created on: 2016年6月29日
 *      Author: max
 */

#include "MatchServerTest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

void recv_callback(struct ev_loop *loop, ev_io *w, int revents);
void async_recv_callback(struct ev_loop *loop, ev_async *w, int revents);

class RequestRunnable : public KRunnable {
public:
	RequestRunnable(MatchServerTest* container) {
		mContainer = container;
	}
	virtual ~RequestRunnable() {
	}
protected:
	void onRun() {
		mContainer->RequestRunnableHandle();
	}
private:
	MatchServerTest* mContainer;
};

class RespondRunnable : public KRunnable {
public:
	RespondRunnable(MatchServerTest* container) {
		mContainer = container;
	}
	virtual ~RespondRunnable() {
	}
protected:
	void onRun() {
		mContainer->RespondRunnableHandle();
	}
private:
	MatchServerTest* mContainer;
};

MatchServerTest::MatchServerTest() {
	// TODO Auto-generated constructor stub
	mIsRunning = false;

	mIp = "";
	mPort = 0;
	mTotalRequest = 0;
	mCurrentRequest = 0;

	mLoop = NULL;

	mpRequestRunnable = new RequestRunnable(this);
	mpRespondRunnable = new RespondRunnable(this);
}

MatchServerTest::~MatchServerTest() {
	// TODO Auto-generated destructor stub
	if( mpRequestRunnable ) {
		delete mpRequestRunnable;
		mpRequestRunnable = NULL;
	}

	if( mpRespondRunnable ) {
		delete mpRespondRunnable;
		mpRespondRunnable = NULL;
	}
}

void MatchServerTest::Start(
		const string& ip,
		unsigned short port,
		unsigned int iTotalRequest,
		unsigned int threadRequestCount
		) {

	printf("# MatchServerTest::Start( "
			"ip : %s, "
			"port : %u, "
			"iTotalRequest : %d, "
			"threadRequestCount : %d "
			") \n",
			ip.c_str(),
			port,
			iTotalRequest,
			threadRequestCount
			);

	mIsRunning = true;
	mIp = ip;
	mPort = port;
	mTotalRequest = iTotalRequest;
	mThreadRequestCount = threadRequestCount;

	mCurrentRequest = 0;

	mLoop = ev_loop_new(EVFLAG_AUTO);

	// 接收线程
	mRespondThread.start(mpRespondRunnable);

	// 发送线程
//	mpRequestThread.start(mpRequestRunnable);
	mpRequestThread = new KThread*[threadRequestCount];
	for(int i = 0; i < threadRequestCount; i++) {
		mpRequestThread[i] = new KThread();
		mpRequestThread[i]->start(mpRequestRunnable);
	}
}

bool MatchServerTest::Request(int index, char* sendBuffer, unsigned int len) {
	struct sockaddr_in mAddr;
	bzero(&mAddr, sizeof(mAddr));
	mAddr.sin_family = AF_INET;
	mAddr.sin_port = htons(mPort);
	mAddr.sin_addr.s_addr = inet_addr(mIp.c_str());

	int iFlag = 1;
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("# MatchServerTest::Request( Create socket error ) \n");
		return false;
	}

	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&iFlag, sizeof(iFlag));
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));

	if( connect(fd, (struct sockaddr *)&mAddr, sizeof(mAddr)) != -1 ) {
//		printf("# MatchServerTest::Request( Connect ok, index : %d ) \n", index);
		if( send(fd, sendBuffer, strlen(sendBuffer), 0) > 0 ) {
			// 开始监听接收
			mRequestList.PushBack(fd);
			ev_async_send(mLoop, &mAsync_send_watcher);
			return true;
		} else {
			// 关闭连接
			close(fd);
		}
	} else {
		printf("# MatchServerTest::Request( Connect fail ) \n");
	}

	return false;
}

bool MatchServerTest::RequestPublic(int index, char* sendBuffer, unsigned int len) {
	struct sockaddr_in mAddr;
	bzero(&mAddr, sizeof(mAddr));
	mAddr.sin_family = AF_INET;
	mAddr.sin_port = htons(mPort + 1);
	mAddr.sin_addr.s_addr = inet_addr(mIp.c_str());

	int iFlag = 1;
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("# MatchServerTest::RequestPublic( Create socket error ) \n");
		return false;
	}

	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&iFlag, sizeof(iFlag));
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &iFlag, sizeof(iFlag));

	if( connect(fd, (struct sockaddr *)&mAddr, sizeof(mAddr)) != -1 ) {
//		printf("# MatchServerTest::RequestPublic( Connect ok, index : %d ) \n", index);
		if( send(fd, sendBuffer, strlen(sendBuffer), 0) > 0 ) {
			// 开始监听接收
			mRequestList.PushBack(fd);
			ev_async_send(mLoop, &mAsync_send_watcher);
			return true;
		} else {
			// 关闭连接
			close(fd);
		}
	} else {
		printf("# MatchServerTest::RequestPublic( Connect fail ) \n");
	}

	return false;
}

void MatchServerTest::RequestRunnableHandle() {
	while( mIsRunning ) {
		mRequestKMutex.lock();
		if( mCurrentRequest < mTotalRequest ) {
			char sendBuffer[2048] = {'\0'};
			memset(sendBuffer, '\0', 2048);

			int r = mCurrentRequest % 6;
			int limit = 8;

//			time_t t;
//			long long unixTime = time(&t);

			switch(r) {
			case 0:{
				// 1.获取跟男士有任意共同答案的问题的女士Id列表接口
//				printf("# MatchServerTest::RequestRunnableHandle( 获取跟男士有任意共同答案的问题的女士Id列表, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s%d HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/QUERY_SAME_ANSWER_LADY_LIST?MANID=CM100&SUBMITTIME=&SITEID=1&LIMIT=",
						limit,
//						unixTime,
						"Keep-alive"
						);
				Request(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			case 1:{
				// 2.获取有指定共同问题的女士Id列表接口
//				printf("# MatchServerTest::RequestRunnableHandle( 获取有指定共同问题的女士Id列表, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s%d HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/QUERY_THE_SAME_QUESTION_LADY_LIST?QID=QA0&SITEID=1&LIMIT=",
						limit,
						"Keep-alive"
						);
				Request(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			case 2:{
				// 3.获取有任意共同问题的女士Id列表接口
//				printf("# MatchServerTest::RequestRunnableHandle( 获取有任意共同问题的女士Id列表, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s%d HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/QUERY_ANY_SAME_QUESTION_LADY_LIST?MANID=CM100&SITEID=1&LIMIT=",
						limit,
						"Keep-alive"
						);
				Request(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			case 3:{
				// 4.获取回答过注册问题的在线女士Id列表接口
//				printf("# MatchServerTest::RequestRunnableHandle( 获取回答过注册问题的在线女士Id列表, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s%d HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/QUERY_SAME_QUESTION_ONLINE_LADY_LIST?QIDS=QA0%2cQA1%2cQA2%2cQA3%2cQA4%2cQA5%2cQA6%2cQA7&SITEID=1&LIMIT=",
						limit,
						"Keep-alive"
						);
				Request(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			case 4:{
				// 5.获取指定问题有共同答案的女士Id列表接口
//				printf("# MatchServerTest::RequestRunnableHandle( 获取指定问题有共同答案的女士Id列表, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s%d HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/QUERY_SAME_QUESTION_ANSWER_LADY_LIST?QIDS=QA0%2cQA1%2cQA2%2cQA3%2cQA4%2cQA5%2cQA6%2cQA7&AIDS=0,0,0,0,0,0,0,0&SITEID=1&LIMIT=",
						limit,
						"Keep-alive"
						);
				Request(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			case 5:{
				// 6.获取回答过注册问题的女士Id列表接口
//				printf("# MatchServerTest::RequestRunnableHandle( 获取回答过注册问题的女士Id列表, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s%d HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/QUERY_SAME_QUESTION_LADY_LIST?QIDS=QA0%2cQA1%2cQA2%2cQA3%2cQA4%2cQA5%2cQA6%2cQA7&SITEID=1&LIMIT=",
						limit,
						"Keep-alive"
						);
				Request(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			case 6:{
				// 7.同步数据库接口
//				printf("# MatchServerTest::RequestRunnableHandle( 同步数据库, index : %d ) \n", mCurrentRequest);
				snprintf(sendBuffer, 2048 - 1, "GET %s HTTP/1.0\r\nConection: %s\r\n\r\n",
						"/SYNC",
						"Keep-alive"
						);
				RequestPublic(mCurrentRequest, sendBuffer, strlen(sendBuffer));
				break;
			}
			}

			mCurrentRequest++;
			mRequestKMutex.unlock();

		} else {
			mIsRunning = false;
			mRequestKMutex.unlock();
			printf("# MatchServerTest::RequestRunnableHandle( Request %d times fnish ) \n", mCurrentRequest);
			break;
		}
	}
}

void MatchServerTest::RespondRunnableHandle() {
	// 开始主循环
	ev_io_start(mLoop, &mAccept_watcher);
	ev_set_userdata(mLoop, this);

	ev_async_init(&mAsync_send_watcher, async_recv_callback);
	ev_async_start(mLoop, &mAsync_send_watcher);

	ev_run(mLoop, 0);
}

void recv_callback(struct ev_loop *loop, ev_io *w, int revents) {
	MatchServerTest *pContainer = (MatchServerTest *)ev_userdata(loop);
	pContainer->RecvCallback(w, revents);
}

void MatchServerTest::RecvCallback(ev_io *w, int revents) {
//	printf("# MatchServerTest::RecvCallback( fd : %d ) \n", w->fd);
	// 输出返回
	char recvBuffer[2048] = {'\0'};
	memset(recvBuffer, '\0', 2048);

	if( !(revents & EV_ERROR) ) {
		int ret = recv(w->fd, recvBuffer, 2048 - 1, 0);
		if( ret > 0 ) {
//			printf("# MatchServerTest::RecvCallback( [%d bytes] :\n%s\n) \n", ret, recvBuffer);
		} else if( ret < 0 ) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				return;
			}
		}
	}

	// 关闭socket
	close(w->fd);
	ev_io_stop(mLoop, w);
	free(w);
}

void async_recv_callback(struct ev_loop *loop, ev_async *w, int revents) {
	MatchServerTest *pContainer = (MatchServerTest *)ev_userdata(loop);
	pContainer->AsyncRecvCallback();

}

void MatchServerTest::AsyncRecvCallback() {
	while( !mRequestList.Empty() ) {
		int fd = mRequestList.PopFront();
		ev_io *watcher = (ev_io *)malloc(sizeof(ev_io));
		ev_io_init(watcher, recv_callback, fd, EV_READ);
		ev_io_start(mLoop, watcher);
	}
}
