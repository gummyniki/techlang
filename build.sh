#!/usr/bin/zsh

cd build
cmake ..
cmake --build .

sudo cp techlang /usr/bin/techlang
