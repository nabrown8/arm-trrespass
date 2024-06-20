#!/bin/bash

pid=$(ps aux | grep 'tester' | grep -v 'grep' | awk 'NR==1 {print $2}')

if [[ -n "$pid" ]]; then
    echo "Killing process with PID: $pid"
    kill -9 $pid  
else
    echo "No process found matching 'tester'"
fi
