#!/bin/bash

#
# Download an build from scratch GCC 7.5.0
# 
# @Author: Navarro Torres, Agustín
# @Date: 25/07/2022
#

# Make dirs
mkdir gcc7.5
cd gcc7.5

# Download gcc
curl https://ftp.gnu.org/gnu/gcc/gcc-7.5.0/gcc-7.5.0.tar.xz --output gcc.tar.xz
if [ $? -ne 0 ]; then
    echo " ${RED}ERROR${NC}"
    exit
fi

# Extract gcc
tar -xf gcc.tar.xz
if [ $? -ne 0 ]; then
    echo " ${RED}ERROR${NC}"
    exit
fi

cd gcc-7.5.0

# Download prerequisites
./contrib/download_prerequisites
if [ $? -ne 0 ]; then
    echo " ${RED}ERROR${NC}"
    exit
fi

# Configure as local installation
mkdir bin
./configure --prefix=$(pwd)/bin --enable-languages=all --disable-multilib
if [ $? -ne 0 ]; then
    echo " ${RED}ERROR${NC}"
    exit
fi

# Build
if [[ "$1" == "Y" ]]; then
    make -j $2
else
    make -j
fi
if [ $? -ne 0 ]; then
    echo " ${RED}ERROR${NC}"
    exit
fi

make install
if [ $? -ne 0 ]; then
    echo " ${RED}ERROR${NC}"
    exit
fi
