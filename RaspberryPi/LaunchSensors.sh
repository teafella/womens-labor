#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
cd $DIR

# while true 
# do
# git pull
# export PYTHONPATH="/usr/local/lib/python3.7/dist-packages"
python3 python/main.py
# ./main.out
# done