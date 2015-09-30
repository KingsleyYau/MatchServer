CXXFLAGS =	-O2 -g -Wall -fmessage-length=0
CXXFLAGS +=	-Ilibev -I. -Wunused-variable

LIBS =		libev/.libs/libev.a -lpthread -lsqlite3 

JSONOBJS = 	json_reader.o json_value.o json_writer.o md5.o
OBJS =		server.o KThread.o KLog.o MatchServer.o TcpServer.o MessageList.o RequestManager.o DBManager.o LogManager.o \
			DataParser.o DataHttpParser.o LogFile.o MessageMgr.o
OBJS += 	$(JSONOBJS)
TARGET =	server

DBTEST_OBJS	=	dbtest.o KThread.o DBManager.o DBManagerTest.o 
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
