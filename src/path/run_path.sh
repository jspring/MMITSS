#!/bin/bash

killall -s KILL sendSoftCall
killall -s KILL ab3418commudp
killall -s KILL nc

xterm -e /bin/nc -lkd localhost 56008 >/dev/null &
xterm -e /bin/nc -lkd localhost 56004 | /bin/nc -lkd localhost 56009 &
#xterm -e ./ab3418commudp 

#echo Ready
#./sendSoftCall
