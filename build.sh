#!/usr/bin/sh

set -e

sdl_headers_path=/usr/include/SDL2

gcc -std=c99 -Wall \
	-DSDL_DISABLE_IMMINTRIN_H -I"$sdl_headers_path" \
	-o countdown countdown.c -lSDL2
