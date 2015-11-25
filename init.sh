#!/bin/bash

if [ $EUID -ne 0 ]; then
    >&2 echo "This script should be run as root."
    exit 1
fi

