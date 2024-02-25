#!/bin/bash

rm compilelog.log

exec 3>&1 4>&2
trap 'exec 2>&4 1>&3' 0 1 2 3
exec 1>compilelog.log 2>&1
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/home/$USER/server -DTOOLS=1 -DWITH_WARNINGS=0
make -j $(nproc)
make install
cd ..
rm -rf build
