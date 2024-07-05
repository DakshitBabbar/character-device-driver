#!/bin/bash

#test1
if [ $1 -eq 1 ]; then
	./writer.o "we the people of india "
	./writer.o "having solemnly resolved "
	./writer.o "to constitute india into a "
	sleep 1
	./reader.o 50 > output1.log 2>&1 & \
	./reader.o 50 > output2.log 2>&1 & \
	./reader.o 50 > output3.log 2>&1 &
	sleep 1
	./writer.o "sovereign socialist secular "

#test2
elif [ $1 -eq 2 ]; then
	./reader.o 25 > output1.log 2>&1 & \
	./reader.o 25 > output2.log 2>&1 & \
	./reader.o 25 > output3.log 2>&1 &
	sleep 2
	./writer.o "we the people of india having solemnly resolved"
	sleep 5
	./writer.o "to constitute india into a "

#test3
elif [ $1 -eq 3 ]; then 
	./writer.o "we the people of india having solemnly resolved"
	sleep 1
	./reader.o 25 > output1.log 2>&1 & \
	./writer.o "to constitute india into a " & \
	./reader.o 25 > output2.log 2>&1 & \
	./reader.o 25 > output3.log 2>&1 &
	 
#test4
#multiple writers together
elif [ $1 -eq 4 ]; then
	./writer.o "we the people of india " & \
	./writer.o "having solemnly resolved " & \
	./writer.o "to constitute india into a " &
	sleep 1
	./reader.o 50 > output1.log 2>&1 & \
	./reader.o 50 > output2.log 2>&1 &

#test5
#multiple readers and writers together on an empty stack
elif [ $1 -eq 5 ]; then
	./reader.o 25 > output1.log 2>&1 & \
	./writer.o "to constitute india into a " & \
	./reader.o 25 > output2.log 2>&1 & \
	./reader.o 25 > output3.log 2>&1 & \
	./writer.o "we the people of india having solemnly resolved" &


#test6
#reader on an empty stack
elif [ $1 -eq 6 ]; then
	./reader.o 25 > output1.log 2>&1 & \
	sleep 2 & \
	./writer.o "to constitute india into a " &

#test7
#reader and writer together on an empty stack
elif [ $1 -eq 7 ]; then
	./reader.o 25 > output1.log 2>&1 & \
	./writer.o "to constitute india into a " &

fi
