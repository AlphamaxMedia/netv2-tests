#!/bin/sh

pulseaudio --start --log-target=syslog  # root user doesn't have pulse audio running...

/home/pi/code/netv2-tests/bin/dut-off.sh  # make sure power is off to jig on start

/home/pi/.cargo/bin/exclave -c /home/pi/code/netv2-tests
