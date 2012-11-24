#!/bin/bash

brew install sdl

cd include/glm-0.3.1/
./configure
make install
cd ../..

gcc gl.cpp $(sdl-config --cflags --libs) -framework OpenGL -lglm -ljpeg -lpng
./a.out
