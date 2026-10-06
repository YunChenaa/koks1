#!/bin/bash


CROSS=~/Inlock/pc/toolchain/mips-gcc540-glibc222-r3.3.7.mxu2.cve2/bin/mips-linux-gnu-

pushd demo/src
  make
popd

