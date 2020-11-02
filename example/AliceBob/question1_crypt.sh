#!/bin/sh
rm -f instance/Bob/receiver/test.txt
rm -f instance/transfer/test.txt
rm -f instance/Alice/sender/aspect_policy.cfg
rm -f instance/Bob/receiver/aspect_policy.cfg
rm -f instance/transfer/aspect_policy.cfg
yes|cp instance/Alice/sender/connector_config.cfg.crypt instance/Alice/sender/connector_config.cfg 
yes|cp instance/Bob/receiver/connector_config.cfg.crypt instance/Bob/receiver/connector_config.cfg 
yes|cp instance/transfer/plugin_config.cfg.teardrop instance/transfer/plugin_config.cfg 
yes|cp instance/transfer/aspect_policy.cfg.teardrop instance/transfer/aspect_policy.cfg 
yes|cp instance/Alice/sender/aspect_policy.cfg.symmcrypt instance/Alice/sender/aspect_policy.cfg 
yes|cp instance/Bob/receiver/aspect_policy.cfg.symmcrypt instance/Bob/receiver/aspect_policy.cfg 

killall -9 envset_proc
sleep 1
sh run_cube.sh exec_def/question1/_Alice_crypt.def &
sh run_cube.sh exec_def/question1/_Bob_crypt.def &
sh run_cube.sh exec_def/question1/_receiver.def &
sh run_cube.sh exec_def/question1/_transfer.def &
sleep 1
sh run_cube.sh exec_def/question1/_sender.def filesend.msg &
