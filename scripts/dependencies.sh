#!/bin/bash

cd scripts
chmod u+x deploy.sh
chmod u+x set_ips.sh
cd ../

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
cd ../../

. ./scripts/deploy.sh