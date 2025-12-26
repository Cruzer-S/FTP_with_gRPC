#!/bin/bash

BASE=$(realpath ${0})
echo "BASE: ${BASE}"

if [ "$1" == "Server" ]; then
	${BASE}/build/Server/Server --root-dir=${BASE} --loglevel 5 127.0.0.1 1584
elif [ "$1" == "Client" ]; then
	rm -f "${BASE}/Resources/image.iso"
	rm -f "${BASE}/Resources/image_copy.iso"
	cp ${BASE}/build/Client/Client ${BASE}/build/Client/Client_copy
	${BASE}/build/Client/Client 127.0.0.1 1584 							\
								${BASE}/Resources/image.iso				\
								${BASE}/Resources/image_copy.iso
fi