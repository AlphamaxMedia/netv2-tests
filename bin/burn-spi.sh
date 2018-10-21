#!/bin/sh

if [ ! -f /tmp/devicetype.txt ]
then
    echo "Device type file not present, must run getidcode.sh first!"
    exit 1
fi

DEVICE=`cat /tmp/devicetype.txt`

if [ "$DEVICE" = "35T" ]
then
    FPGAIMAGE="user-35.bit"
    BSCANIMAGE="bscan_spi_xc7a35t.bit"
elif [ "$DEVICE" = "100T" ]
then
    FPGAIMAGE="user-100.bit"
    BSCANIMAGE="bscan_spi_xc7a100t.bit"
else
    echo "Device type not valid, aborting!"
    exit 1
fi

# convert the firmware bin into an uploadable blob
rm -f /tmp/ufirmware.upl
rm -f /tmp/ufirmware.bin

# pad the firmware out to fill out the full firmware area
# the reason is that ufirmware.bin sometimes is not divisible by 4, which will
# cause the CRC computation to fail. So this forces a padding on ufirmware.bin
# which guarantees a deterministic fill for the entire firmware length and
# thus allow CRC to succeed
cp /home/pi/code/netv2-tests/bin/netv2-fpga-userimage/production-images/user-firmware.bin /tmp/ufirmware.bin
dd if=/dev/zero of=/tmp/ufirmware.bin bs=1 count=1 seek=131071

/home/pi/code/netv2-tests/bin/netv2-fpga/bin/mknetv2img -f --output /tmp/ufirmware.upl /tmp/ufirmware.bin

echo "Burning tester firmware image into flash..."
sudo /opt/openocd/bin/openocd \
     -c 'set FIRMWARE_FILE /tmp/ufirmware.upl' \
     -c "set BSCAN_FILE /home/pi/code/netv2-tests/bin/netv2mvp-scripts/$BSCANIMAGE" \
     -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/cl-firmware.cfg


echo "Loading tester FPGA bitstream via JTAG..."
sudo /opt/openocd/bin/openocd \
     -c "set FPGAIMAGE /home/pi/code/netv2-tests/bin/netv2-fpga-userimage/production-images/$FPGAIMAGE" \
     -c "set BSCAN_FILE /home/pi/code/netv2-tests/bin/netv2mvp-scripts/$BSCANIMAGE" \
     -f /home/pi/code/netv2-tests/bin/netv2mvp-scripts/cl-spifpga.cfg

sleep 3 # give a moment for the bios to boot, so flterm doesn't catch it by accident
echo "Final FPGA code loaded..."

exit 0
