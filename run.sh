#!/bin/bash

if [ "$1" == "Server" ]; then
	rm -f ./build/Server/test.txt
	./build/Server/Server --root-dir=./build/Server/ --loglevel 5 127.0.0.1 1584
elif [ "$1" == "Client" ]; then
	rm -f /home/user/mythos/ftp_with_grpc/build/Client/Client_copy
	rm -f /home/user/mythos/ftp_with_grpc/build/Client/Client_copy2
	cp /home/user/mythos/ftp_with_grpc/build/Client/Client /home/user/mythos/ftp_with_grpc/build/Client/Client_copy
	./build/Client/Client 127.0.0.1 1584 												\
						  /home/user/mythos/ftp_with_grpc/build/Client/Client_copy 		\
						  /home/user/mythos/ftp_with_grpc/build/Client/Client_copy2
fi