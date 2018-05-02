#!/bin/sh

func1()
{
    modprobe tiny_tty
    ls -l /dev/ttyPL*
    modprobe -r tiny_tty
    ls -l /dev/ttyPL*
}


func2() {
    insmod tiny_tty.ko
    sleep 0.5
    ls -l /dev/ttyPL*
    cat /proc/tty/driver/pl_uart_tty
    rmmod tiny_tty
    sleep 0.5
    ls -l /dev/ttyPL*
    cat /proc/tty/driver/pl_uart_tty
}

for i in $(seq 1 $1 )
do
    echo "[$i]"
    func2
done

