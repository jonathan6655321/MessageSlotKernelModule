/*
 * message_sender.c
 *
 *  Created on: Jun 21, 2017
 *      Author: Jonathan
 */


#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {

	int channelIndex;
	sscanf(argv[1], "%d", &channelIndex);
	if (channelIndex > NUM_CHANNELS - 1 || channelIndex < 0) {
		printf("Error: wrong channel index argument");
		return -1;
	}

	char message[MESSAGE_BUFFER_LENGTH];


	int file_desc, ret_val;

	// if you want multiple files, add path as arg 3
	if (argc == 3)
	{
		char devicePath[250] = "/dev/";
		strcat(devicePath, argv[2]);
		file_desc = open(devicePath, O_RDWR, 0666);
			if (file_desc < 0) {
				printf("Can't open device file: %s %s\n",
				devicePath,strerror(errno));
				return -1;
			}
	}
	else
	{
		file_desc = open("/dev/"DEVICE_FILE_NAME, O_RDWR, 0666);
		if (file_desc < 0) {
			printf("Can't open device file: %s %s\n",
			DEVICE_FILE_NAME,strerror(errno));
			return -1;
		}
	}


	ret_val = ioctl(file_desc, IOCTL_SET_ENC, channelIndex);

	if (ret_val < 0) {
		printf("ioctl_set_msg failed:%d\n", ret_val);
		return -1;
	}

	ret_val = read(file_desc, message, MESSAGE_BUFFER_LENGTH);
	if (ret_val < 0) {
			printf("read failed:%d %s\n", ret_val, strerror(errno));
			return -1;
		}

	close(file_desc);

	printf("read message: \n%s\n\n from channel number: %d\n", message, channelIndex);
	return 0;

}
