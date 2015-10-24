# Copyright (C) 2015 The QpidNetwork
# MatchServer Makefile
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

export MAKE	:=	make

debug=1
ifeq ($(debug), 1)
CXXFLAGS =	-O2 -g
else
CXXFLAGS =  -O3 
endif

CXXFLAGS +=	-O2 -g -Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-I. -Isqlite -Ilibev -I/usr/include/mysql

LIBS =		-L. \
			-Wl,-Bstatic -Llibev/.libs -lev \
			-Wl,-Bstatic -Lsqlite/.libs -lsqlite3 \
			-Wl,-Bdynamic -L/usr/lib64/mysql -L/usr/lib/mysql -lmysqlclient \
			-Wl,-Bdynamic -ldl -lz -lpthread 

JSONOBJS = 	json_reader.o json_value.o json_writer.o md5.o
OBJS =		server.o KThread.o KLog.o MatchServer.o TcpServer.o MessageList.o \
			DataParser.o DataHttpParser.o DBManager.o DBSpool.o LogManager.o LogFile.o \
			ConfFile.o Arithmetic.o 
OBJS += 	$(JSONOBJS)
TARGET =	matchserver

DBTEST_OBJS	=	dbtest.o KThread.o DBManager.o DBManagerTest.o DBSpool.o LogManager.o LogFile.o MessageList.o \
				ConfFile.o Arithmetic.o 
DBTEST_OBJS += 	$(JSONOBJS)
DBTEST_TARGET =		dbtest

CLIENTTEST_OBJS	=	client.o KThread.o TcpTestClient.o
CLIENTTEST_TARGET =		client

DEPDIRS	:= sqlite libev
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps test clean cleanall install $(DEPDIRS) $(TARGET) $(DBTEST_TARGET) $(CLIENTTEST_TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid matchserver completed!'
	@echo ''
	@echo '################################################################'

$(DEPDIRS):
	$(MAKE) -C $@
	
$(CLEAN_DEPS):	
	$(MAKE) -C $(patsubst _clean_%, %, $@) clean
	
$(DBTEST_TARGET):	$(DBTEST_OBJS)
	$(CXX) -o $(DBTEST_TARGET) $(DBTEST_OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid dbtest completed!'
	@echo ''
	@echo '################################################################'

$(CLIENTTEST_TARGET):	$(CLIENTTEST_OBJS)
	$(CXX) -o $(CLIENTTEST_TARGET) $(CLIENTTEST_OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid client completed!'
	@echo ''
	@echo '################################################################'
	
deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps completed!'
	@echo ''
	@echo '################################################################'
	
all:	deps $(TARGET) $(DBTEST_TARGET) $(CLIENTTEST_TARGET)

test:	$(DBTEST_TARGET) $(CLIENTTEST_TARGET)
	
clean:
	rm -f $(OBJS) $(TARGET) $(DBTEST_TARGET) $(DBTEST_OBJS) $(CLIENTTEST_TARGET) $(CLIENTTEST_OBJS)

cleanall: clean	$(CLEAN_DEPS) 
	@echo '################################################################'
	@echo ''
	@echo '# Clean all finished!'
	@echo ''
	@echo '################################################################'
	
install: 
	copy matchserver.config /etc/ -rf
	copy matchserver /usr/local/bin
	chmod +x /usr/local/bin/matchserver
	@echo '################################################################'
	@echo ''
	@echo '# Install matcherserver finished!'
	@echo ''
	@echo '################################################################'
