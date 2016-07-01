/*
 * Client.cpp
 *
 *  Created on: 2015-11-10
 *      Author: Max
 */

#include "Client.h"

Client::Client() {
	// TODO Auto-generated constructor stub
	fd = -1;
	isOnline = false;
    ip = "";
	index = -1;
	starttime = 0;

    mBuffer.Reset();

    miSendingPacket = 0;
    miSentPacket = 0;

    miDataPacketRecvSeq = 0;

	mpClientCallback = NULL;
	mpIdleMessageList = NULL;

	mbError = false;
}

Client::~Client() {
	// TODO Auto-generated destructor stub
	mpIdleMessageList = NULL;
}

void Client::SetClientCallback(ClientCallback* pClientCallback) {
	mpClientCallback = pClientCallback;
}

void Client::SetMessageList(MessageList* pIdleMessageList) {
	mpIdleMessageList = pIdleMessageList;
}

int Client::ParseData(Message* m)  {
	int ret = 0;

	char* buffer = m->buffer;
	int len = m->len;
	int seq = m->seq;

	mKMutex.lock();

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseData( "
			"tid : %d, "
			"[收到客户端数据包], "
			"miDataPacketRecvSeq : %d, "
			"seq : %d "
			")",
			(int)syscall(SYS_gettid),
			miDataPacketRecvSeq,
			seq
			);

	if( mbError ) {
		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseData( "
				"tid : %d, "
				"[客户端数据包已经解析错误, 不再处理] "
				")",
				(int)syscall(SYS_gettid)
				);

		ret = -1;
	}

	if( ret != -1 ) {
		if( miDataPacketRecvSeq != seq ) {
			// 收到客户端乱序数据包
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseData( "
					"tid : %d, "
					"[收到客户端乱序的数据包] "
					")",
					(int)syscall(SYS_gettid)
					);

			// 缓存到队列
			Message* mc = mpIdleMessageList->PopFront();
			if( mc != NULL ) {
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"Client::ParseData( "
						"tid : %d, "
						"[缓存客户端乱序的数据包到队列] "
						")",
						(int)syscall(SYS_gettid)
						);

				if( len > 0 ) {
					memcpy(mc->buffer, buffer, len);
				}

				mc->len = len;
				mMessageMap.Insert(seq, mc);

			} else {
				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"Client::ParseData( "
						"tid : %d, "
						"[客户端没有缓存空间] "
						")",
						(int)syscall(SYS_gettid)
						);

				ret = -1;
			}

		} else {
			// 收到客户端顺序的数据包
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseData( "
					"tid : %d, "
					"[收到客户端顺序的数据包] "
					")",
					(int)syscall(SYS_gettid)
					);

			ret = 1;
		}

		// 开始解析数据包
		if( ret == 1 ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseData( "
					"tid : %d, "
					"[开始解析客户端当前数据包] "
					")",
					(int)syscall(SYS_gettid)
					);

			// 解当前数据包
			ret = ParseCurrentPackage(m);
			if( ret != -1 ) {
				miDataPacketRecvSeq++;

				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"Client::ParseData( "
						"tid : %d, "
						"[开始解析客户端缓存数据包] "
						")",
						(int)syscall(SYS_gettid)
						);

				// 解析缓存数据包
				MessageMap::iterator itr;
				while( true ) {
					itr = mMessageMap.Find(miDataPacketRecvSeq);
					if( itr != mMessageMap.End() ) {
						// 找到对应缓存包
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"Client::ParseData( "
								"tid : %d, "
								"[找到对应缓存包], "
								"miDataPacketRecvSeq : %d "
								")",
								(int)syscall(SYS_gettid),
								miDataPacketRecvSeq
								);

						Message* mc = itr->second;
						ret = ParseCurrentPackage(mc);

						mpIdleMessageList->PushBack(mc);
						mMessageMap.Erase(itr);

						if( ret != -1 ) {
							miDataPacketRecvSeq++;

						} else {
							break;
						}

					} else {
						// 已经没有缓存包
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"Client::ParseData( "
								"tid : %d, "
								"[已经没有缓存包]"
								")",
								(int)syscall(SYS_gettid)
								);
						break;
					}
				}
			}
		}
	}

	if( ret == -1 ) {
		if( !mbError ) {
			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"Client::ParseData( "
					"tid : %d, "
					"[客户端数据包解析错误] "
					")",
					(int)syscall(SYS_gettid)
					);

		}

		mbError = true;
	}

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseData( "
			"tid : %d, "
			"[解析客户端数据包完成], "
			"ret : %d "
			")",
			(int)syscall(SYS_gettid),
			ret
			);

	mKMutex.unlock();

	return ret;
}

int Client::ParseCurrentPackage(Message* m) {
	int ret = 0;

	char* buffer = m->buffer;
	int len = m->len;
	index = m->index;
	starttime = m->starttime;

	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseCurrentPackage( "
			"tid : %d, "
			"[解析当前数据包], "
			"miDataPacketRecvSeq : %d, "
			"index : %d "
			")",
			(int)syscall(SYS_gettid),
			miDataPacketRecvSeq,
			index
			);

	int last = len;
	int recved = 0;
	while( ret != -1 && last > 0 ) {
		int recv = (last < MAX_BUFFER_LEN - mBuffer.len)?last:(MAX_BUFFER_LEN - mBuffer.len);
		if( recv > 0 ) {
			memcpy(mBuffer.buffer + mBuffer.len, buffer + recved, recv);
			mBuffer.len += recv;
			last -= recv;
			recved += recv;
		}

		LogManager::GetLogManager()->Log(
				LOG_STAT,
				"Client::ParseCurrentPackage( "
				"tid : %d, "
				"[解析数据], "
				"mBuffer.len : %d, "
				"recv : %d, "
				"last : %d, "
				"len : %d "
				")",
				(int)syscall(SYS_gettid),
				mBuffer.len,
				recv,
				last,
				len
				);

		// 解析命令
		DataHttpParser parser;
		int parsedLen = parser.ParseData(mBuffer.buffer, mBuffer.len);
		while( parsedLen != 0 ) {
			if( parsedLen != -1 ) {
				// 解析成功

				// 解析命令
				ParseCommand(&parser);

				// 替换数据
				mBuffer.len -= parsedLen;
				if( mBuffer.len > 0 ) {
					memcpy(mBuffer.buffer, mBuffer.buffer + parsedLen, mBuffer.len);
				}

				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"Client::ParseCurrentPackage( "
						"tid : %d, "
						"[替换数据], "
						"mBuffer.len : %d "
						")",
						(int)syscall(SYS_gettid),
						mBuffer.len
						);

				ret = 1;

				// 继续解析
//				parsedLen = parser.ParseData(mBuffer.buffer, mBuffer.len);
				// 短连接只解析一次
				break;

			} else {
				// 解析错误
				ret = -1;
				break;
			}
		}
	}

	LogManager::GetLogManager()->Log(
			LOG_MSG,
			"Client::ParseCurrentPackage( "
			"tid : %d, "
			"[解析当前数据包], "
			"ret : %d "
			")",
			(int)syscall(SYS_gettid),
			ret
			);

	return ret;
}

void Client::ParseCommand(DataHttpParser* pDataHttpParser) {
	LogManager::GetLogManager()->Log(
			LOG_STAT,
			"Client::ParseCommand( "
			"tid : %d, "
			"[解析命令], "
			"pDataHttpParser->GetPath() : %s "
			")",
			(int)syscall(SYS_gettid),
			pDataHttpParser->GetPath().c_str()
			);
	if( pDataHttpParser->GetPath() == "/QUERY_SAME_ANSWER_LADY_LIST" ) {
		// 1.获取跟男士有任意共同答案的问题的女士Id列表接口
		if( mpClientCallback != NULL ) {
			const char* pManId = pDataHttpParser->GetParam("MANID").c_str();
			const char* pSiteId = pDataHttpParser->GetParam("SITEID").c_str();
			const char* pLimit = pDataHttpParser->GetParam("LIMIT").c_str();
			const char* pSubmitTime = pDataHttpParser->GetParam("SUBMITTIME").c_str();
			long long iRegTime = 0;
			if( pSubmitTime != NULL ) {
				iRegTime = atoll(pSubmitTime);
			}

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:1.获取跟男士有任意共同答案的问题的女士Id列表接口] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientQuerySameAnswerLadyList(this, pManId, pSiteId, pLimit, iRegTime);
		}
	} else if( pDataHttpParser->GetPath() == "/QUERY_THE_SAME_QUESTION_LADY_LIST" ) {
		// 2.获取有指定共同问题的女士Id列表接口
		if( mpClientCallback != NULL ) {
			const char* pQId = pDataHttpParser->GetParam("QID").c_str();
			const char* pSiteId = pDataHttpParser->GetParam("SITEID").c_str();
			const char* pLimit = pDataHttpParser->GetParam("LIMIT").c_str();

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:2.获取有指定共同问题的女士Id列表接口] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientQueryTheSameQuestionLadyList(this, pQId, pSiteId, pLimit);
		}
	} else if( pDataHttpParser->GetPath() == "/QUERY_ANY_SAME_QUESTION_LADY_LIST" ) {
		// 3.获取有任意共同问题的女士Id列表接口
		if( mpClientCallback != NULL ) {
			const char* pManId = pDataHttpParser->GetParam("MANID").c_str();
			const char* pSiteId = pDataHttpParser->GetParam("SITEID").c_str();
			const char* pLimit = pDataHttpParser->GetParam("LIMIT").c_str();

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:3.获取有任意共同问题的女士Id列表接口] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientQueryAnySameQuestionLadyList(this, pManId, pSiteId, pLimit);
		}
	} else if( pDataHttpParser->GetPath() == "/QUERY_SAME_QUESTION_ONLINE_LADY_LIST" ) {
		// 4.获取回答过注册问题的在线女士Id列表接口
		if( mpClientCallback != NULL ) {
			const char* pQIds = pDataHttpParser->GetParam("QIDS").c_str();
			const char* pSiteId = pDataHttpParser->GetParam("SITEID").c_str();
			const char* pLimit = pDataHttpParser->GetParam("LIMIT").c_str();

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:4.获取回答过注册问题的在线女士Id列表接口] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientQuerySameQuestionOnlineLadyList(this, pQIds, pSiteId, pLimit);
		}
	} else if( pDataHttpParser->GetPath() == "/QUERY_SAME_QUESTION_ANSWER_LADY_LIST" ) {
		// 5.获取指定问题有共同答案的女士Id列表接口
		if( mpClientCallback != NULL ) {
			const char* pQIds = pDataHttpParser->GetParam("QIDS").c_str();
			const char* pAIds = pDataHttpParser->GetParam("AIDS").c_str();
			const char* pSiteId = pDataHttpParser->GetParam("SITEID").c_str();
			const char* pLimit = pDataHttpParser->GetParam("LIMIT").c_str();

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:5.获取指定问题有共同答案的女士Id列表接口] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientQuerySameQuestionAnswerLadyList(this, pQIds, pAIds, pSiteId, pLimit);
		}
	} else if( pDataHttpParser->GetPath() == "/QUERY_SAME_QUESTION_LADY_LIST" ) {
		// 6.获取回答过注册问题的女士Id列表接口
		if( mpClientCallback != NULL ) {
			const char* pQIds = pDataHttpParser->GetParam("QIDS").c_str();
			const char* pSiteId = pDataHttpParser->GetParam("SITEID").c_str();
			const char* pLimit = pDataHttpParser->GetParam("LIMIT").c_str();

			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:6.获取回答过注册问题的女士Id列表接口] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientQuerySameQuestionLadyList(this, pQIds, pSiteId, pLimit);
		}
	}  else if( pDataHttpParser->GetPath() == "/SYNC" ) {
		// 外部接口, 同步数据库
		if( mpClientCallback != NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:6.同步数据库] "
					")",
					(int)syscall(SYS_gettid)
					);

			mpClientCallback->OnClientSync(this);
		}
	} else {
		if( mpClientCallback != NULL ) {
			LogManager::GetLogManager()->Log(
					LOG_STAT,
					"Client::ParseCommand( "
					"tid : %d, "
					"[解析命令:未知命令] "
					")",
					(int)syscall(SYS_gettid)
					);
			mpClientCallback->OnClientUndefinedCommand(this);
		}
	}
}

int Client::AddSendingPacket() {
	int seq;
	mKMutexSend.lock();
	seq = miSendingPacket++;
	mKMutexSend.unlock();
	return seq;
}

int Client::AddSentPacket() {
	int seq;
	mKMutexSend.lock();
	seq = miSentPacket++;
	mKMutexSend.unlock();
	return seq;
}

bool Client::IsAllPacketSent() {
	bool bFlag = false;
	mKMutexSend.lock();
	if( miSentPacket == miSendingPacket ) {
		bFlag = true;
	}
	mKMutexSend.unlock();
	return bFlag;
}
