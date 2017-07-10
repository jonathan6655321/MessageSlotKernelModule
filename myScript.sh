#!/bin/bash

sudo rmmod message_slot
sudo rm /dev/message_slot

sudo make
echo ++++++++++++++++++++ INSERTING MODULE +++++++++++++++
sudo insmod message_slot.ko
sudo mknod /dev/message_slot c 245 0

gcc -o message_sender message_sender.c
gcc -o message_reader message_reader.c
echo +++++++++++++++++ SENDING MESSAGE ++++++++++++++++++
./message_sender 1 hello
echo +++++++++++++++++++++++ READING MESSAGE ++++++++++++++++++
./message_reader 1

echo +++++++++++++++++ SENDING MESSAGE ++++++++++++++++++
./message_sender 2 avergjnlakdmakdmsfdhfgjfghdfgh
echo +++++++++++++++++++++++ READING MESSAGE ++++++++++++++++++
./message_reader 1
echo +++++++++++++++++ SENDING MESSAGE ++++++++++++++++++
./message_sender 2  channel2HOMie!!!
echo +++++++++++++++++++++++ READING MESSAGE ++++++++++++++++++
./message_reader 2
#echo +++++++++++++++++ KERNEL LOG: +++++++++++++++++
#dmesg | tail -20
