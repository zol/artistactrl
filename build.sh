#!/bin/sh

ARTISTA_LIB=/tmp/artista
ARTISTA_INC=/tmp/artista

#static artista
#g++ -o aristactrl aristactrl.cpp -I$ARTISTA_INC/include -L$ARTISTA_LIB/lib -static -lartista -lpthread -lusb -lrt `Magick++-config --cppflags --cxxflags --ldflags --libs`

#shared
g++ -o artistactrl artistactrl.cpp -I$ARTISTA_INC/include -L$ARTISTA_LIB/lib -lartista `Magick++-config --cppflags --cxxflags --ldflags --libs`
