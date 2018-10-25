#!/bin/sh

PWR_SWITCH=26

gpio -g mode ${PWR_SWITCH} out

gpio -g write ${PWR_SWITCH} 0
