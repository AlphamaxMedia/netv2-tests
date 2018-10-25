[Trigger]
Name=PCI hot plug trigger
Description=Trigger on hot plug event.  Compile with 'gcc hotplugpoll.c -o hotplugpoll; sudo chown root hotplugpoll; sudo chmod u+s hotplugpoll'
ExecStart=/home/pi/code/netv2-tests/bin/hotplugpoll 6
