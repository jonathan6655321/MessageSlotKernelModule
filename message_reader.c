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

int main(int argc, char **argv) {

	int channelIndex = argv[1];
	if (channelIndex > NUM_CHANNELS - 1 || channelIndex < 0) {
		printf("Error: wrong channel index argument");
		return -1;
	}

	char message[MESSAGE_BUFFER_LENGTH];


	int file_desc, ret_val;

	file_desc = open("/dev/"DEVICE_FILE_NAME, 0);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n",
		DEVICE_FILE_NAME);
		exit(-1);
	}

	ret_val = ioctl(file_desc, IOCTL_SET_ENC, *channelIndex);

	if (ret_val < 0) {
		printf("ioctl_set_msg failed:%d\n", ret_val);
		exit(-1);
	}

	ret_val = read(file_desc, message, MESSAGE_BUFFER_LENGTH);
	if (ret_val < 0) {
			printf("write failed:%d\n", ret_val);
			exit(-1);
		}

	close(file_desc);

	printf("read message: \n%s\n\n from channel number: %d", message, channelIndex);
	return 0;

}
