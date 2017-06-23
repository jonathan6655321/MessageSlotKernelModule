/*
 * message_slot.h
 *
 *  Created on: Jun 21, 2017
 *      Author: Jonathan
 */

#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_


// FROM CHARDEV.H:


#include <linux/ioctl.h>



/* The major device number. We can't rely on dynamic
 * registration any more, because ioctls need to know
 * it. */
#define MAJOR_NUM 245
#define NUM_CHANNELS 4
#define MESSAGE_BUFFER_LENGTH 128
#define SUCCESS 0
#define FAIL -1

/* Set the message of the device driver */
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define DEVICE_FILE_NAME "message_slot"


//// FROM CHARDEV.H: ^^^^^^^^^^^^^^^




#endif /* MESSAGE_SLOT_H_ */
