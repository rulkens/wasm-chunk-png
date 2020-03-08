#!/usr/bin/env bash
cmake . &&
cmake --build . &&
ls -lh docs | grep chunk_png | awk '{ print $5 "\t" $9 }'
