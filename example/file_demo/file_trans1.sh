#!/bin/sh
rm file_sender/aspect_policy.cfg
cp file_receiver/connector_config.cfg.direct file_receiver/connector_config.cfg
rm file_receiver/test.txt

killall envset_proc
sleep 1

sh ../run_cube.sh exec_def/_file_receiver.def &
sleep 1
sh ../run_cube.sh exec_def/_file_sender.def request.txt &
