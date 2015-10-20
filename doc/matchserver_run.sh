# !/bin/sh
#
# Copyright (C) 2015 The QpidNetwork
# MatchServer run shell
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#
PROCESS="/root/MatchServer/matchserver -f /root/MatchServer/matchserver.config"
PID=`ps -ef | grep -w matchserver | grep -v "grep" | awk '{ print $2 }'`
if [ -z "$PID" ];then
	echo "restart matchserver." 
	`$PROCESS`
fi