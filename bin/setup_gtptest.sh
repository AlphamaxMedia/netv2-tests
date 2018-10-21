#!/bin/sh

if [ ! -f /tmp/devicetype.txt ]
then
    echo "Device type file not present, must run getidcode.sh first!"
    exit 1
fi

DEVICE=`cat /tmp/devicetype.txt`

if [ "$DEVICE" = "35T" ]
then
    TESTIMAGE="gtptester-35.bit"
    BSCANIMAGE="bscan_spi_xc7a35t.bit"
elif [ "$DEVICE" = "100T" ]
then
    TESTIMAGE="gtptester-100.bit"
    BSCANIMAGE="bscan_spi_xc7a100t.bit"
else
    echo "Device type not valid, aborting!"
    exit 1
fi

# convert the firmware bin into an uploadable blob
rm -f /tmp/firmware.upl
rm -f /tmp/firmware.bin

# pad the firmware out to fill out the full firmware area
# the reason is that firmware.bin sometimes is not divisible by 4, which will
# cause the CRC computation to fail. So this forces a padding on firmware.bin
# which guarantees a deterministic fill for the entire firmware length and
# thus allow CRC to succeed
cp /home/pi/code/netv2-tests/bin/netv2-fpga/tester-images/gtptester-firmware.bin /tmp/firmware.bin
dd if=/dev/zero of=/tmp/firmware.bin bs=1 count=1 seek=131071

/home/pi/code/netv2-tests/bin/netv2-fpga/bin/mknetv2img -f --output /tmp/firmware.upl /tmp/firmware.bin

# drop in the initial tester firmware image (so we don't have to flterm it)
echo "Burning tester firmware image into flash..."
sudo /opt/openocd/bin/openocd \
     -c 'set FIRMWARE_FILE /tmp/firmware.upl' \
     -c "set BSCAN_FILE /home/pi/code/netv2-tests/bin/netv2mvp-scripts/$BSCANIMAGE" \
     -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/cl-firmware.cfg


echo "Loading tester FPGA bitstream via JTAG..."
sudo /opt/openocd/bin/openocd \
     -c "set BITFILE /home/pi/code/netv2-tests/bin/netv2-fpga/tester-images/$TESTIMAGE" \
     -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/cl-fpga.cfg

sleep 3 # give a moment for the bios to boot, so flterm doesn't catch it by accident
echo "GTP test firmware loaded..."

exit 0
