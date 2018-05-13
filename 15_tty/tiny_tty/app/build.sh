#!/bin/bash

PWD=$(cd "$(dirname "$0")";pwd)

rm -rf ${PWD}/app

arm-linux-gcc decode_termios.c app.c -o app

