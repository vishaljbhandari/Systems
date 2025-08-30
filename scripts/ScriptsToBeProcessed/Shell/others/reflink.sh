#!/bin/bash
dd if=/dev/zero of=bigfile bs=1M count=190
time cp --reflink=auto bigfile bigfile.2
