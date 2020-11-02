#!/bin/sh
rm file_sender/aspect_policy.cfg
rm file_receiver/aspect_policy.cfg
cp file_transfer/aspect_policy.cfg.teardrop file_transfer/aspect_policy.cfg
rm file_receiver/test.txt
rm file_transfer/test.txt
cp file_receiver/connector_config.cfg.transfer file_receiver/connector_config.cfg

killall envset_proc
sleep 1

sh ../run_cube.sh exec_def/_file_receiver.def &
sleep 1
sh ../run_cube.sh exec_def/_file_transfer.def &
sleep 1
sh ../run_cube.sh exec_def/_file_sender.def request.txt &
