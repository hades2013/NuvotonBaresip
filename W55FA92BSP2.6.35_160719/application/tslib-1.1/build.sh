#!/bin/bash
./autogen.sh
./configure --host=arm-linux --prefix=$PWD/_tslib_install
make && make install
