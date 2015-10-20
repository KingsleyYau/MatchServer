# !/bin/bash
# Copyright (C) 2015 The QpidNetwork
# MatchServer Configure
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#
# chkconfig:   2345 20 80
# description: matchserver
# processname: /etc/rc.d/init.d/matchserver

PID_FILE=/var/run/matchserver.pid
DAEMON=/usr/local/bin/matchserver
EXEC=matchserver

# Source function library.
. /etc/rc.d/init.d/functions
 
if ! [ -x $DAEMON ] ; then
       echo "ERROR: $DAEMON not found"
       exit 1
fi
 
stop()
{
       echo "Stoping $EXEC ..."
       killall $DAEMON >/dev/null
       usleep 100
       echo "Shutting down $EXEC: [  OK  ]"     
}
 
start()
{
       echo "Starting $EXEC ..."
       $DAEMON > /dev/null &
       usleep 100
       echo "Starting $EXEC: [  OK  ]"        
}
 
restart()
{
       stop
       start
}
 
 
case "$1" in
       start)
       start
       ;;
       stop)
       stop
       ;;
       restart)
       restart
       ;;
       status)
       status -p $PID_FILE $DAEMON 
       ;;   
  *)
       echo "Usage: service $EXEC {start|stop|restart|status}"
       exit 1
esac
 
exit $?