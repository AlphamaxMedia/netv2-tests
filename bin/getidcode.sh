#!/bin/bash

rm -f /tmp/devicetype.txt

#DEBUG=`sudo /opt/openocd/bin/openocd -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/idcode.cfg 2>&1`
#echo $DEBUG

IDCODE=`sudo /opt/openocd/bin/openocd -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/idcode.cfg 2>&1 | grep -E -o "tap/device found: [0-9a-z]+" | cut -f3 -d" "`

echo "Got $IDCODE"

if [ "$IDCODE" = "0x0362d093" ]
then
    echo "Device type is 35T"
    echo "35T" > /tmp/devicetype.txt
elif [ "$IDCODE" = "0x03631093" ]
then
    echo "Device type is 100T"
    echo "100T" > /tmp/devicetype.txt
else
    echo "Device type is unknown or invalid"
    echo "INVAILD" > /tmp/devicetype.txt
    exit 1
fi

exit 0
