#!/bin/bash

rm -rf output  build

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

cmake --build build

rm -rf ~/code/internal/engine/modules/bl_vcs/libs/rtc/*

cp -rf output/* ~/code/internal/engine/modules/bl_vcs/libs/rtc/
