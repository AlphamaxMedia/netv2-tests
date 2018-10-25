#!/bin/sh

PWR_SWITCH=26
PROG_N=24

gpio -g mode ${PROG_N} out
gpio -g write ${PROG_N} 1   # make sure the "prog" pin doesn't accidentally hang low

gpio -g mode ${PWR_SWITCH} out
gpio -g write ${PWR_SWITCH} 1
