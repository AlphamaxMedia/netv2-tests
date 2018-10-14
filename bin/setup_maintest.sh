#!/bin/sh

if [ ! -f /tmp/devicetype.txt ]
then
    echo "Device type file not present, must run getidcode.sh first!"
    exit 1
fi

DEVICE=`cat /tmp/devicetype.txt`

if [ "$DEVICE" = "35T" ]
then
    TESTIMAGE="tester-35.bit"
elif [ "$DEVICE" = "100T" ]
then
    TESTIMAGE="tester-100.bit"
else
    echo "Device type not valid, aborting!"
    exit 1
fi

# convert the firmware bin into an uploadable blob
/home/pi/code/netv2-tests/bin/netv2-fpga/bin/mknetv2img -f --output /tmp/firmware.upl /home/pi/code/netv2-tests/bin/netv2-fpga/tester-images/tester-firmware.bin

# drop in the initial tester firmware image (so we don't have to flterm it)
echo "Burning tester firmware image into flash..."
sudo /opt/openocd/bin/openocd \
     -c 'set FIRMWARE_FILE /tmp/firmware.upl' \
     -c 'set BSCAN_FILE /home/pi/code/netv2-tests/bin/netv2mvp-scripts/bscan_spi_xc7a35t.bit' \
     -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/cl-firmware.cfg


echo "Loading tester FPGA bitstream via JTAG..."
sudo /opt/openocd/bin/openocd \
     -c "set BITFILE /home/pi/code/netv2-tests/bin/netv2-fpga/tester-images/$TESTIMAGE" \
     -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/cl-fpga.cfg

echo "Don't forget to wait until the REPL prompt is reached before issuing further commands..."

exit 0
