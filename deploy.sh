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
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Codefellas/shared/Debug/
ldd consola

cd ../../kernel/Debug
make clean
make all
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Codefellas/shared/Debug/
ldd kernel

cd ../../cpu/Debug
make clean
make all
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Codefellas/shared/Debug/
ldd cpu

cd ../../memoria/Debug
make clean
make all
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Codefellas/shared/Debug/
ldd memoria

cd ../../fileSystem/Debug
make clean
make all
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/tp-2023-1c-Codefellas/shared/Debug/
ldd fileSystem

cd ../../