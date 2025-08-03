#!/bin/sh

echo ">>> Building the engine library"
cd engine/
./build.sh
cd ../

echo ">>> Building the testapp"
cd testapp/
./build.sh
cd ../

echo ">>> Completed"
