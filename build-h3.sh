#!/bin/bash

if [ -z $1 ]; then
    echo 'Please provide install prefix'
    exit
fi

INSTALL_PREFIX="$1"
LIBH3_DIR="h3"

if [[ ! -z $2 ]]; then
    LIBH3_DIR="$2"
fi

LIBH3_BUILD_DIR="${LIBH3_DIR}/build"

mkdir -p "$LIBH3_BUILD_DIR"
cd "$LIBH3_BUILD_DIR" \
&& cmake -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_C_FLAGS="-fPIC -fvisibility=hidden" \
   -DBUILD_TESTING=OFF \
   -DENABLE_COVERAGE=OFF \
   -DENABLE_DOCS=OFF \
   -DENABLE_FORMAT=OFF \
   -DENABLE_LINTING=OFF \
   -DCMAKE_INSTALL_PREFIX:PATH="$INSTALL_PREFIX" \
   .. \
&& cmake --build . --target h3 \
&& cmake --build . --target install \
&& cd -
