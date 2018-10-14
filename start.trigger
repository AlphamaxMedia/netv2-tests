[Trigger]
Name=Start button
Description=A start button on the jig, pin 25.  Compile with 'gcc gpiopoll.c -o gpiopoll; sudo chown root gpiopoll; sudo chmod u+s gpiopoll'
ExecStart=/home/pi/code/netv2-tests/bin/gpiopoll 25
