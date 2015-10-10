CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-Ilibev -I. -I/usr/include/mysql 

LIBS =		libev/.libs/libev.a -lpthread -L. -lsqlite3 -L/usr/lib64/mysql -lmysqlclient

JSONOBJS = 	json_reader.o json_value.o json_writer.o md5.o
OBJS =		server.o KThread.o KLog.o MatchServer.o TcpServer.o MessageList.o RequestManager.o  \
			DataParser.o DataHttpParser.o MessageMgr.o DBManager.o DBSpool.o LogManager.o LogFile.o
OBJS += 	$(JSONOBJS)
TARGET =	server

DBTEST_OBJS	=	dbtest.o KThread.o DBManager.o DBManagerTest.o DBSpool.o LogManager.o LogFile.o MessageList.o
DBTEST_TARGET =		dbtest

CLIENTTEST_OBJS	=	client.o KThread.o TcpTestClient.o
CLIENTTEST_TARGET =		client

all:	$(TARGET) $(DBTEST_TARGET) $(CLIENTTEST_TARGET)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

$(DBTEST_TARGET):	$(DBTEST_OBJS)
	$(CXX) -o $(DBTEST_TARGET) $(DBTEST_OBJS) $(LIBS)

$(CLIENTTEST_TARGET):	$(CLIENTTEST_OBJS)
	$(CXX) -o $(CLIENTTEST_TARGET) $(CLIENTTEST_OBJS) $(LIBS)
	
clean:
	rm -f $(OBJS) $(TARGET) $(DBTEST_TARGET) $(DBTEST_OBJS) $(CLIENTTEST_TARGET) $(CLIENTTEST_OBJS)
