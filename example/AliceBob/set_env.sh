#!/bin/bash
export CUBEAPPPATH=`pwd` 
export CUBE_APP_PLUGIN=$CUBEAPPPATH/plugin/:$CUBE_APP_PLUGIN
export CUBE_DEFINE_PATH=$CUBEAPPPATH/define:$CUBE_DEFINE_PATH
export CUBE_TCM_PATH=/root/centoscloud/cube-tcm
export CUBE_TCM_PLUGIN=/root/centoscloud/cube_tcmplugin
