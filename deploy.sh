#!/bin/bash

cd ../
git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
make debug
make install
cd ../

sudo cp tp-2023-1c-Codefellas/shared/include/* ../../usr/local/include/
cd tp-2023-1c-Codefellas/shared/Debug
make clean
make all
sudo cp libshared.so ../../../../../usr/local/lib/

cd ../../consola/Debug
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

cd ../../
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Codefellas/shared/Debug/