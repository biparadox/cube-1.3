#!/bin/sh
killall envset_proc
sleep 1

sh ../run_cube.sh exec_def/_file_receiver.def &
sleep 1
sh ../run_cube.sh exec_def/_file_sender.def request.txt &
