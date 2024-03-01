#!/bin/sh
# Custom clean command
if [ "$1" = "--clean" ]; then
  rm -rf build
  echo "Done"
else
  mkdir -p build
  cd build
  cmake ..
  make
fi
