#!/bin/sh
rm -f instance/Bob/receiver/test.txt
rm -f instance/Alice/sender/aspect_policy.cfg
rm -f instance/Bob/receiver/aspect_policy.cfg

killall -9 envset_proc
sleep 1
sh run_cube.sh exec_def/question1/_file_receiver.def &
sh run_cube.sh exec_def/question1/_file_transfer.def &
sleep 1
sh run_cube.sh exec_def/question1/_file_sender.def filesend.msg
