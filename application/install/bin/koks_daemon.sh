#!/bin/sh

koks &
PID=$!

wait $PID
exit_status=$?
if [ $exit_status -ne 0 ]; then
    echo "koks exit badly"
    reboot
fi
