#!/bin/bash
make clean
./aesdchar_unload
make
./aesdchar_load
../assignment-autotest/test/assignment8/drivertest.sh
make clean
./aesdchar_unload
make
sudo ./aesdchar_load
../server/aesdsocket
../assignment-autotest/test/assignment8/sockettest.sh
make clean