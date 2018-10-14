#!/bin/sh

blue_led=8
yellow_led=7
red_led=1

all_off() {
	gpio -g write ${blue_led} 0
	gpio -g write ${yellow_led} 0
	gpio -g write ${red_led} 0
}

blue_on() {
	all_off
	gpio -g write ${blue_led} 1
}

yellow_on() {
	all_off
	gpio -g write ${yellow_led} 1
}

red_on() {
	all_off
	gpio -g write ${red_led} 1
}

red_also_on() {
	gpio -g write ${red_led} 1
}

gpio_setup() {
	gpio -g mode ${blue_led} out
	gpio -g mode ${yellow_led} out
	gpio -g mode ${red_led} out
	blue_on
}

gpio_setup

echo "HELLO bash-ltc-jig 1.0"
blue_on
sleep .5
yellow_on
sleep .5
red_on
sleep .5
yellow_on
sleep .5
blue_on

while read line
do
	if echo "${line}" | grep -iq '^start'
	then
		yellow_on
	elif echo "${line}" | grep -iq '^fail'
	then
		red_also_on
	elif echo "${line}" | grep -iq '^finish'
	then
		result=$(echo ${line} | awk '{print $3}')
		if [ ${result} -ge 200 -a ${result} -lt 300 ]
		then
			blue_on
		else
			red_on
		fi
	elif echo "${line}" | grep -iq '^exit'
	then
		exit 0
	fi
done
