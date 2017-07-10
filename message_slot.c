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
#include<linux/init.h>

#include <stddef.h>

#define GET_FILE_ID file->f_inode->i_ino
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

void initMessageNode(MessageNode * mNode)
{
	(*mNode).nextNode = NULL;
	(*mNode).deviceFileId = UNINITIALIZED;
	initMessageSlot(&((*mNode).messageSlot));
}


static MessageNode * head;


/*
 * get node pointer by asssociated unique device file id
 */
MessageNode * getNode(int deviceFileId, MessageNode **newNodeP) {
	if(head == NULL)
	{
		return NULL;
	}
	else
	{
		(*newNodeP) = head; // avoid dereferencing null
	}
	printk("************* head is not null\n");

	while ((**newNodeP).deviceFileId != deviceFileId) {
		printk("the current device file id: %d", (**newNodeP).deviceFileId);
		if ((**newNodeP).nextNode == NULL) {
			*newNodeP = NULL;
			return NULL;
		}
		else
		{
			*newNodeP = (**newNodeP).nextNode;
		}
	}
	return *newNodeP;
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
	MessageNode *newNodeP;
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

	printk(KERN_EMERG "************* 1\n");

	if ((getNode(GET_FILE_ID, &newNodeP)) == NULL)
	{
		printk(KERN_EMERG "************* 2\n");
		if (head == NULL) {
			printk(KERN_EMERG "************* 3\n");
			head = kmalloc(sizeof(MessageNode), GFP_KERNEL); // TODO check if failed?
			initMessageNode(head); // init first! then set file id
			head->deviceFileId = GET_FILE_ID;
			printk("a node was created with file id: %d\n", head->deviceFileId);
		}
		else
		{
			printk(KERN_EMERG "************* 4\n");
			newNodeP = kmalloc(sizeof(MessageNode), GFP_KERNEL); // TODO check if failed?
			initMessageNode(newNodeP);
			newNodeP->deviceFileId = GET_FILE_ID;
			(*newNodeP).nextNode = head;
			head = newNodeP;
			printk("a node was created with file id: %d\n", newNodeP->deviceFileId);
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

	spin_unlock_irqrestore(&device_info.lock, flags);

	return SUCCESS;
}

/* a process which has already opened
 the device file attempts to read from it */
static ssize_t device_read(struct file *file, char * buffer, size_t length,
		loff_t * offset) {
	int i;

	// TODO check user space buffer?
	MessageNode *newNodeP = NULL;
	MessageNode mNode;
	if (getNode(GET_FILE_ID, &newNodeP) == NULL)
	{
		printk("getNode failed to find node with file id: %ld", GET_FILE_ID);
		return -1;
	}
	else
	{
		mNode = *newNodeP;
	}
	if (mNode.messageSlot.currentChannelIndex == UNINITIALIZED) {
		printk("Tried to read but channel index uninitialized\n");
		return -1;
	}

	for (i = 0; i < length && i < MESSAGE_BUFFER_LENGTH; i++) {
		put_user(mNode.messageSlot.messageBufferlArray[mNode.messageSlot.currentChannelIndex].message[i], buffer + i);
	}

	return i; // invalid argument error
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char * buffer,
		size_t length, loff_t * offset) {
	int i;

	// TODO check user space buffer?
	MessageNode *newNodeP = NULL;
	if(getNode(GET_FILE_ID, &newNodeP) == NULL)
	{
		printk("could not find node in write\n");
		return -1;
	}

	if ((*newNodeP).messageSlot.currentChannelIndex == UNINITIALIZED) {
		printk("Tried to write but channel index uninitialized\n");
		return -1;
	}



	for (i = 0; i < MESSAGE_BUFFER_LENGTH; i++) {
		if (i < length)
		{
			get_user((*newNodeP).messageSlot.messageBufferlArray[(*newNodeP).messageSlot.currentChannelIndex].message[i], buffer + i);
		}
		else
		{
			(*newNodeP).messageSlot.messageBufferlArray[(*newNodeP).messageSlot.currentChannelIndex].message[i] = '\0'; // TODO what do they mean by zero?
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



	MessageNode *newNodeP = NULL;
	getNode(GET_FILE_ID, &newNodeP);
	// TODO check correct command received? ? ? ? ?
	printk("in ioctl, the param is %ld\n", ioctl_param);


	if (ioctl_param > 3 || ioctl_param < 0) {
		printk("Wrong ioctl argument. channel index invalid\n");
		return -1;
	}
	(*newNodeP).messageSlot.currentChannelIndex = ioctl_param;

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

	// init vars:
	head = NULL;

//	printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
//	printk("If you want to talk to the device driver,\n");
//	printk("you have to create a device file:\n");
//	printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
//	printk("You can echo/cat to/from the device file.\n");
//	printk("Dont forget to rm the device file and rmmod when you're done\n");

	return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void simple_cleanup(void)
{
	/*
	 * Unregister the device
	 * should always succeed (didnt used to in older kernel versions)
	 */

	MessageNode *currentNode;
	MessageNode *prevNode;

//	 release memory:
	currentNode = head;
	do {
		prevNode = currentNode;
		currentNode = currentNode->nextNode;
		kfree(prevNode);
	} while (currentNode != NULL);

	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init( simple_init);
module_exit( simple_cleanup);
