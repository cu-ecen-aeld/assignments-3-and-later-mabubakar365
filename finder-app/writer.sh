#!/bin/sh

writefile=$1
writestr=$2

dir=$(dirname "$writefile")

# echo "$writefile"
# echo "$writestr"
# echo "$dir"

if [ $# -le 1 ]
    then
        # echo "wrong number of parameters"
        exit 1

elif [ -z "$writefile" ]
    then
        # echo "parameter 1 is empty"
        exit 1

elif [ -z "$writestr" ]
    then
        # echo "parameter 2 is empty"
        exit 1
# else
    # echo ""
fi 

if [ -d "$dir" ]
    then
        if [ -f "$writefile" ] 
            then
                # echo "file exists"
                echo "$writestr" > "$writefile"
        else 
            touch "$writefile"
            if [ -f "$writefile" ]
                then
                    echo "$writestr" > "$writefile"
            else
                # echo "file creation failed"
                exit 1
            fi
        fi
else
    mkdir -p "$dir"
    if [ -d "$dir" ]
        then
            touch "$writefile"
            if [ -f "$writefile" ]
                then 
                    echo "$writestr" > "$writefile"  
            # else
                # echo "file creation failed"
            fi
    fi
fi