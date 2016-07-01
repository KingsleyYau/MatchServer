# Copyright (C) 2015 The QpidNetwork
# MatchServer Makefile
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

export MAKE	:=	make

release=0
ifeq ($(release), 1)
CXXFLAGS = -O3 
else
CXXFLAGS = -O2 -g
endif

CXXFLAGS +=	-Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-I. -Isqlite -Ilibev -I/usr/include/mysql

LIBS =		-L. \
			-Wl,-Bstatic -Llibev/.libs -lev \
			-Wl,-Bstatic -Lsqlite/.libs -lsqlite3 \
			-Wl,-Bdynamic -L/usr/lib64/mysql -L/usr/lib/mysql -lmysqlclient \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMON  	=	common/LogFile.o common/md5.o common/KThread.o common/ConfFile.o common/Arithmetic.o\
				common/IAutoLock.o common/CommonFunc.o common/DBSpool.o
				
JSON    	=	json/json_reader.o json/json_value.o json/json_writer.o

OBJS =		server.o LogManager.o Client.o\
			MatchServer.o TcpServer.o MessageList.o DataParser.o DataHttpParser.o DBManager.o
			
OBJS +=		$(COMMON)
OBJS +=		$(JSON)			
			
TARGET =	matchserver

DEPDIRS	:= sqlite libev
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps clean cleanall install $(DEPDIRS) $(TARGET)

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
	
deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps completed!'
	@echo ''
	@echo '################################################################'
	
all:	deps $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

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
