#!/bin/sh
WHOAMI=`whoami`
PID=`ps -u $WHOAMI|grep miniFtpd|awk '{print $1}'`
if (test "$1" = "") then
	echo "Usage:miniFtp [start][stop][version]..."
	exit 0
fi
if (test "$1" = "start") then
        if (test "$PID" = "") then
	./miniFtpd
	fi
        exit 0
fi
if (test "$1" = "stop") then
        if (test "$PID" != "") then
        kill $PID
        fi
        exit 0
fi
if (test "$1" = "version") then
	echo "miniFtpd 1.0 `date` by wangkun"
        exit 0
fi
echo "Usage:miniFtp [start][stop][version]..."
