#!/bin/sh

cd bin
./zasm ../Examples/hello_world.zasm -dinfo -o ../../zvm/Examples/test.zexec  
cd .. 
