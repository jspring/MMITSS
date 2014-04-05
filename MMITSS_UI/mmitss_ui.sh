#!/bin/bash

if [[ $1 == "start" ]] 
then
	sudo killall ab3418comm
	sudo killall db_slv
	
	/home/path/db/lnx/db_slv &
	sleep 1

	hostname | grep taylor
	if [[ $? != 0 ]]
	then
		sudo /home/atsc/ab3418/lnx/ab3418comm -u -c -v -p /dev/ttyUSB0 | grep message &
	else
		sudo /home/atsc/ab3418/lnx/ab3418comm -u -c -v &
	fi
	sleep 1

	ps -elf | grep MMITSS_Display | grep Xojo | grep -v grep
	if [[ $? != 0 ]]
	then
		sudo /opt/xojo/xojo2014r1/Xojo /opt/xojo/xojo2014r1/Extras/Plugins\ SDK/Examples/MMITSS/MMITSS_UI/MMITSS_Display.xojo_binary_project
	fi
fi

if [[ $1 == "stop" ]] 
then
	sudo killall ab3418comm
	sudo killall db_slv
fi
