# Copyright (C) 2015 The CamShareMiddleware
# test client Makefile
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
CXXFLAGS +=	-I. -I../ -I../libev

LIBS =		-L. \
			-Wl,-Bstatic -L../libev/.libs -lev \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMON  	=	../common/LogFile.o ../common/md5.o ../common/KThread.o ../common/ConfFile.o ../common/Arithmetic.o\
				../common/IAutoLock.o ../common/CommonFunc.o
				
OBJS	+= $(COMMON)		
OBJS	+=	client.o MatchServerTest.o
		
TARGET =	client

.PHONY: clean $(TARGET)

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid client completed!'
	@echo ''
	@echo '################################################################'

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)