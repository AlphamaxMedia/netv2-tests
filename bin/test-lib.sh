#!/bin/sh -e
gpio_dir=/sys/class/gpio
#reset_pulse=34
reset_level=12
#servo_pwm=36
#light_sensor=37
all_pins="${reset_level}"
uart=/dev/ttyAMA0
#uart=/dev/ttyUSB0
baud=115200
error_count=0

status_white=19
status_blue=13
status_yellow=6


## most of this is cruft below from BM17 jig, cull it once the tests are done

pin_to_gpio() {
	local pin_num="$1"
	case "${pin_num}" in
		${reset_level}) echo ${reset_level} ;;
		*) echo "Unrecognized pin: ${pin_num}" ; exit 1; ;;
	esac
}

unexport_pin() {
	local pin_num=$(pin_to_gpio "$1")
	if [ -e "${gpio_dir}/gpio${pin_num}" ]
	then
		echo none > ${gpio_dir}/gpio${pin_num}/edge 2> /dev/null
		echo in > ${gpio_dir}/gpio${pin_num}/direction
		echo ${pin_num} > /sys/class/gpio/unexport
	fi
}

export_pin() {
	local pin_num=$(pin_to_gpio "$1")
	if [ -e "${gpio_dir}/gpio${pin_num}" ]
	then
		echo ${pin_num} > /sys/class/gpio/unexport
	fi

	if [ ! -e "${gpio_dir}/gpio${pin_num}" ]
	then
		echo ${pin_num} > /sys/class/gpio/export
	fi
}

set_input() {
	local pin_num=$(pin_to_gpio "$1")
	echo "in" > ${gpio_dir}/gpio${pin_num}/direction
}

set_output() {
	local pin_num=$(pin_to_gpio "$1")
	echo "out" > ${gpio_dir}/gpio${pin_num}/direction
}

set_low() {
	local pin_num=$(pin_to_gpio "$1")
	echo 0 > ${gpio_dir}/gpio${pin_num}/value
}

set_high() {
	local pin_num=$(pin_to_gpio "$1")
	echo 1 > ${gpio_dir}/gpio${pin_num}/value
}

get_value() {
	local pin_num=$(pin_to_gpio "$1")
	set_input "$1"
	if [ $(cat ${gpio_dir}/gpio${pin_num}/value) = 0 ]
	then
		return 0
	fi
	return 1
}

enter_programming_mode() {
	set_input ${reset_pulse}
	set_output ${reset_level}
	set_low ${reset_level}
	sleep 1.5
	set_input ${reset_level}
}

reset_board() {
	set_output 26
	set_low 26
	sleep .1
	set_input 26
}

wait_for_banner() {
	grep -q "TEST START" ${uart}
}

wait_for_green_on() {
	until get_value ${status_green}
	do
		sleep 0.1
	done
}

wait_for_green_off() {
	until ! get_value ${status_green}
	do
		sleep 0.1
	done
}

wait_for_red_on() {
	until get_value ${status_red}
	do
		sleep 0.1
	done
}

wait_for_red_off() {
	until ! get_value ${status_red}
	do
		sleep 0.1
	done
}

