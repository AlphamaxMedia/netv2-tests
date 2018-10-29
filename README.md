# netv2-tests

Factory tests for NeTV2.

# Overview

"Full scenario" requires the following cables and hardware to be installed:

* TX0->Overlay
* TX1->RX0
* PCI express loopback header
* Rpi loopback/breakout
* Ping test network cable
* USB loopback
* A microSD card
* A fan

The hardware design that accompanies this test program can be
downloaded at: https://github.com/AlphamaxMedia/netv2-testhat

This will check every port and every function. The test requires two
passes because a "fully-loaded" FPGA at the current litex rev has
difficulty routing clocks to four GTP interfaces (it can route to one
easily, and two with some finagling). Some optimization of PLL resource
usage and clocks globally would alleviate this, but to make testing
expedient we do one test where we test all the features, and then
a second, quick GTP-only test to confirm all four GTP channels are
soldered correctly.

Once the test completes, it also burns the production firmware into
the SPI ROM.

The test is meant to be triggered automatically when the PCI
cards are detected as plugged in.

The test directory itself is structured to refer to all of the
helper JTAG scripts & firmware images through submodules. The
netv-fpga repository is actually included twice, but one on the
"master" branch, and other on the "tester-master" branch. Each
of these include a copy of the production binary, and the method
for updating the tester is thus to do a git pull of the alphamax
repository and pull in the latest published binary.

"Quick scenario" requires no cables to be installed, but it does
assume you have a jig which can measure all the voltage test points on
the PCB. Quick scenario confirms about 80% of the solder connections
and also quickly shuts down the board if any voltages are out of spec,
thus preventing further damage.

The quick scenario is meant to be a fast check of the PCB to screen
gross solder issues before burning the labor and time to run the full
scenario. It's meant to be triggered using a mechanical pushbutton.

This is meant to be run using the "exclave" tester infrastructure,
which you can get here:

https://github.com/exclave/exclave

And there's an exclave.service file which you can use to install
it as a boot service, which means your Rpi will boot into a tester
automatically.

This tester depends on the system having a number of libraries
installed, including "expect", pulseaudio, and a web interface module
which can be downloaded at
https://github.com/AlphamaxMedia/jig-20-interface-http. The openocd
scripts built into the submodule assume you have the alphamax fork of
openocd built and installed, which is necessary because of some
critical bugs in the bcm2835 driver that have been submitted to the
maintainers for mainline inclusion but it's unclear if it will ever be
incorporated. There are also some gpio helper scripts that need
to be compiled and setuid to run correctly; those instructions
are documented in the ".trigger" files. 

A final note is that the scripts include hard-coded paths, because
the earlier version of exclave used during development has no
option to fix the run path (so the relative path is shifting
depending upon where you start exclave, and this is annoying
especially when you need it to start as a system service file).
Thus the scripts assume you have done a "git clone" of the netv2-tests
repository in a directory called /home/pi/code, that you've pulled
in the submodules. If you change the username, you'll have to
recode all the scripts, unfortunately.
