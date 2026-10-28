#!/bin/sh
set -xe

gcc -std=c11 -O2 -Wall -Wextra -pedantic -o heart heart.c -lm
