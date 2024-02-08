#!/bin/sh

filesdir=$1
searchstr=$2

if [ $# -le 1 ]
    then
        # echo "wrong number of parameters"
        exit 1

elif [ -z "$filesdir" ]
    then
        # echo "parameter 1 is empty"
        exit 1

elif [ -z "$searchstr" ]
    then
        # echo "parameter 2 is empty"
        exit 1

else
    x=$(grep -ro "$searchstr" "$filesdir" | wc -l)
    y=$(grep -rl "$searchstr" "$filesdir" | wc -l)
    echo "The number of files are $y and the number of matching lines are $x in $y"
    exit 0

fi