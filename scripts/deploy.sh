#!/bin/bash

cd consola/Debug
make clean
make all

cd ../../kernel/Debug
make clean
make all

cd ../../cpu/Debug
make clean
make all

cd ../../memoria/Debug
make clean
make all

cd ../../fileSystem/Debug
make clean
make all

cd ../../scripts/
chmod u+x restart_fs.sh
./restart_fs.sh
cd ../
export LD_LIBRARY_PATH=~/tp-2023-1c-Codefellas/shared/Debug/