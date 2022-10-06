#!/bin/bash

mkdir -p m4 autoconf-aux
aclocal
libtoolize
autoconf
touch NEWS AUTHORS ChangeLog
automake --add-missing
