#!/usr/bin/env bash
script_directory=$(dirname $(readlink -f $0))
dd if=/dev/random of=${script_directory}/artifacts/13-b.dat  bs=1c  count=13
dd if=/dev/random of=${script_directory}/artifacts/13-kib.dat  bs=1K count=13
dd if=/dev/random of=${script_directory}/artifacts/13-mib.dat  bs=1M count=13
dd if=/dev/random of=${script_directory}/artifacts/13-gib.dat  bs=1G count=13

