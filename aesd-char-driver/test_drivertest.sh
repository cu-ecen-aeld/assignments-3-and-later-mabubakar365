#!/bin/bash
make clean
./aesdchar_unload
make
./aesdchar_load
../assignment-autotest/test/assignment8/drivertest.sh
make clean