#!/bin/bash
export CUBE_PATH=`pwd`
export CUBELIBPATH=$CUBE_PATH/cubelib
export CUBESYSPATH=$CUBE_PATH/proc
export LD_LIBRARY_PATH=$CUBELIBPATH/lib/:$LD_LIBRARY_PATH
export CUBE_BASE_DEFINE=$CUBE_PATH/proc/main/base_define
export CUBE_DEFINE_PATH=$CUBE_PATH/proc/define
export CUBE_SYS_PLUGIN=$CUBE_PATH/proc/plugin/
#export CUBE_PATH CUBELIBPATH LD_LIBRARY_PATH
