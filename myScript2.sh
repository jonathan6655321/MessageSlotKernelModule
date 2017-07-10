#!/bin/bash

sudo rmmod message_slot
sudo rm /dev/message_slot

sudo make
sudo make
echo ++++++++++++++++++++ INSERTING MODULE +++++++++++++++
sudo insmod message_slot.ko
sudo mknod /dev/message_slot2 c 245 0

gcc -o message_sender message_sender.c
gcc -o message_reader message_reader.c
echo +++++++++++++++++ SENDING MESSAGE ++++++++++++++++++
./message_sender 1 "hello from device 2" message_slot2
echo +++++++++++++++++++++++ READING MESSAGE ++++++++++++++++++
./message_reader 1  message_slot2

echo +++++++++++++++++ SENDING MESSAGE ++++++++++++++++++
./message_sender 2 averZZZZZZZZZZZZZ2222222hfgjfghdfgh  message_slot2
echo +++++++++++++++++++++++ READING MESSAGE ++++++++++++++++++
./message_reader 1  message_slot2 
echo +++++++++++++++++ SENDING MESSAGE ++++++++++++++++++
./message_sender 2  "device 2 is in the HOUSE!!!!"  message_slot2
echo +++++++++++++++++++++++ READING MESSAGE ++++++++++++++++++
./message_reader 2  message_slot2
#echo +++++++++++++++++ KERNEL LOG: +++++++++++++++++
#dmesg | tail -20
