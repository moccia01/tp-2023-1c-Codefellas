#!/bin/bash

cd fileSystem/archivos/
rm bitmap.dat
rm bloques.dat
rm -r fcb/
mkdir fcb
touch bitmap.dat 
touch bloques.dat
truncate -s 4194304 bloques.dat
truncate -s 65536 bitmap.dat