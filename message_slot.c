/*
 * message_slot.c
 *
 *  Created on: Jun 21, 2017
 *      Author: Jonathan
 */
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <stddef.h>

#define GET_FILE_ID (*file)->f_inode->i_ino
#define UNINITIALIZED -1

typedef struct MessageBuffer {
	char message[MESSAGE_BUFFER_LENGTH];
} MessageBuffer;

typedef struct MessageSlot {
	MessageBuffer messageBufferlArray[NUM_CHANNELS]; // needs to be initialized to hold 128 byte buffers
	int currentChannelIndex; // valid indeces: 0-3
} MessageSlot;

void initMessageSlot(MessageSlot *ms)
{
	(*ms).currentChannelIndex = UNINITIALIZED;
}

typedef struct MessageNode {
	int deviceFileId;
	MessageSlot messageSlot;
	struct MessageNode * nextNode;
} MessageNode;

void initMessageNode(MessageNode mNode)
{
	mNode.nextNode = NULL;
	mNode.deviceFileId = UNINITIALIZED;
	initMessageSlot(&(mNode.messageSlot));
}


static MessageNode * head = NULL;


/*
 * get node pointer by asssociated unique device file id
 */
MessageNode * getNode(int deviceFileId) {
	MessageNode currentNode = *head;
	if(currentNode == NULL)
	{
		return NULL;
	}
	while (currentNode.deviceFileId != deviceFileId) {
		if (currentNode.nextNode == NULL) {
			return NULL;
		}

		currentNode = *(currentNode.nextNode);
	}
	return &currentNode;
}

/*******************FROM RECITATION: ***********************************/

MODULE_LICENSE("GPL");

struct chardev_info {
	spinlock_t lock;
};

static int dev_open_flag = 0; /* used to prevent concurent access into the same device */
static struct chardev_info device_info;

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file) {
	unsigned long flags; // for spinlock
	printk("device_open(%p)\n", file);

	/*
	 * We don't want to talk to two processes at the same time
	 */
	spin_lock_irqsave(&device_info.lock, flags);
	if (dev_open_flag) {
		spin_unlock_irqrestore(&device_info.lock, flags);
		return -EBUSY;
	}
	dev_open_flag++;

	// ACTIONS:
	if ( *(getNode(GET_FILE_ID)) == NULL)
	{
		if (head == NULL) {
			head = kmalloc(sizeof(MessageNode), GFP_KERNEL); // TODO check if failed?
			initMessageNode(head); // init first! then set file id
			(*head)->deviceFileId = GET_FILE_ID;
		}
		else
		{
			MessageNode *newNode = kmalloc(sizeof(MessageNode), GFP_KERNEL); // TODO check if failed?
			initMessageNode(newNode);
			(*newNode)->deviceFileId = GET_FILE_ID;
			(*newNode).nextNode = head;
			head = newNode;
		}
	}

	spin_unlock_irqrestore(&device_info.lock, flags);

	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
	unsigned long flags; // for spinlock
	printk("device_release(%p,%p)\n", inode, file);

	/* ready for our next caller */
	spin_lock_irqsave(&device_info.lock, flags);
	dev_open_flag--;


	// release memory:
	MessageNode currentNode = *head;
	MessageNode prevNode;


	// not handling nulls here because a struct with file id HAS to exist
	if ((*head)->deviceFileId == GET_FILE_ID)
	{
		currentNode = *(head->nextNode);
		free(head);
		head = &currentNode;
	}
	else
	{
		prevNode = currentNode;
		currentNode = *(currentNode.nextNode);
		while (currentNode.deviceFileId != GET_FILE_ID)
		{
			prevNode = currentNode;
			currentNode = *(currentNode.nextNode);
		}

		prevNode.nextNode = currentNode.nextNode;
		free(currentNode); // TODO am I doing this right?
	}




	spin_unlock_irqrestore(&device_info.lock, flags);

	return SUCCESS;
}

/* a process which has already opened
 the device file attempts to read from it */
static ssize_t device_read(struct file *file, char * buffer, size_t length,
		loff_t * offset) {

	// TODO check user space buffer?

	MessageNode mNode = *(getNode(GET_FILE_ID));
	if (mNode.messageSlot.currentChannelIndex == UNINITIALIZED) {
		printk("Tried to read but channel index uninitialized\n");
		return -1;
	}

	int i;
	for (i = 0; i < length && i < MESSAGE_BUFFER_LENGTH; i++) {
		put_user(mNode.messageSlot.messageBufferlArray[mNode.messageSlot.currentChannelIndex].message[i], buffer + i);
	}

	return i; // invalid argument error
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char * buffer,
		size_t length, loff_t * offset) {

	// TODO check user space buffer?

	MessageNode mNode = *(getNode(GET_FILE_ID));
	if (mNode.messageSlot.currentChannelIndex == UNINITIALIZED) {
		printk("Tried to read but channel index uninitialized\n");
		return -1;
	}


	printk("device_write(%p,%d)\n", file, length);

	int i;
	for (i = 0; i < MESSAGE_BUFFER_LENGTH; i++) {
		if (i < length)
		{
			get_user(mNode.messageSlot.messageBufferlArray[mNode.messageSlot.currentChannelIndex].message[i], buffer + i);
		}
		else
		{
			mNode.messageSlot.messageBufferlArray[mNode.messageSlot.currentChannelIndex].message[i] = '\0'; // TODO what do they mean by zero?
		}
	}

	/* return the number of input characters used */
	return i;
}

// this supports changing the current channel index TODO I WROTE THIS!
static long device_ioctl( //struct inode*  inode,
		struct file* file, unsigned int ioctl_num,/* The number of the ioctl */
		unsigned long ioctl_param) /* The parameter to it */
{
	/* Switch according to the ioctl called */
//  if(IOCTL_SET_ENC == ioctl_num)
//	{
//    /* Get the parameter given to ioctl by the process */
//		printk("chardev, ioctl: setting encryption flag to %ld\n", ioctl_param);
//    encryption_flag = ioctl_param;
//  }



	// TODO check correct command received? ? ? ? ?



	if (ioctl_param > 3 || ioctl_param < 0) {
		printf("Wrong ioctl argument. channel index invalid\n");
		return -1;
	}

	MessageNode mNode = *(getNode(GET_FILE_ID));
	mNode.messageSlot.currentChannelIndex = ioctl_param;

	return SUCCESS;
}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */

struct file_operations Fops = { .read = device_read, .write = device_write,
		.unlocked_ioctl = device_ioctl, .open = device_open, .release =
				device_release, };

/* Called when module is loaded.
 * Initialize the module - Register the character device */
static int simple_init(void)
{
	unsigned int rc = 0;
	/* init dev struct*/
	memset(&device_info, 0, sizeof(struct chardev_info));
	spin_lock_init(&device_info.lock);

	/* Register a character device. Get newly assigned major num */
	rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

	/*
	 * Negative values signify an error
	 */
	if (rc < 0)
	{
		printk("Error: could not register module\n");
		return -1;
	}

	printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
	printk("If you want to talk to the device driver,\n");
	printk("you have to create a device file:\n");
	printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
	printk("You can echo/cat to/from the device file.\n");
	printk("Dont forget to rm the device file and rmmod when you're done\n");

	return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void simple_cleanup(void)
{
	/*
	 * Unregister the device
	 * should always succeed (didnt used to in older kernel versions)
	 */
	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init( simple_init);
module_exit( simple_cleanup);
