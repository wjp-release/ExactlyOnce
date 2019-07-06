#!/bin/bash

cd ../build
cmake ../src/ -GNinja -DCMAKE_BUILD_TYPE=Debug
ninja