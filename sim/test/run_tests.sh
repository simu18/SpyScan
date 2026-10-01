#!/bin/sh
# PC tests for the hardware-independent modules. Usage: sh run_tests.sh
set -e
cd "$(dirname "$0")"
cc -O2 -Wall -Wextra -o tu test_ui_fsm.c ../wokwi/ui_fsm.c
cc -O2 -Wall -Wextra -o tc test_challenge.c ../wokwi/challenge.c ../wokwi/sim_sources.c -lm
./tu
./tc 12 5000
./tc 8 5000
rm -f tu tc
