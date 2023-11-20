#!/bin/sh

build() {
    [ ! "$(command -v cc)" ] && {
	echo "error: no compiler was found, linked with name \"cc\"."
	exit 1
    }

    cc -Wall -Wextra -O2 -Wno-stringop-overread -march=native -s twc.c -o twc
    [ "$?" -ne 0 ] && {
	echo "error: compilation failed."
	exit 1
    }

    exit 0
}

build
