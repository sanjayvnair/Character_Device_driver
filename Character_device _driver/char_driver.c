
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/moduleparam.h>
#define ramdisk_size (size_t) (16*PAGE_SIZE)
#define REGULAR 0
#define DEVICE "mycdrv"
#define REVERSE 1
#define MYIOC_TYPE 'Z'
static int NUM_DEVICES = 3;
module_param(NUM_DEVICES, int, S_IRUGO);
int char_major;
int char_minor = 0;
static LIST_HEAD(asp_list);
static struct class *foo_class;
static dev_t first;
static unsigned int count = 1;

static int mycdrv_open(struct inode *inode, struct file *file);
static int mycdrv_release(struct inode *inode, struct file *file);
static ssize_t mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos);
static ssize_t mycdrv_write(struct file *file, const char __user * buf, size_t lbuf, loff_t * ppos);
static loff_t mycdrv_lseek(struct file *file, loff_t offset, int orig);
static long mycdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static const struct file_operations mycdrv_fops={
	.owner = THIS_MODULE,
	.read = mycdrv_read,
	.write = mycdrv_write,
	.llseek = mycdrv_lseek,
	.unlocked_ioctl = mycdrv_ioctl,
	.open = mycdrv_open,
	.release = mycdrv_release
};

struct asp_mycdrv{
	struct list_head list;
	struct cdev dev;
	char *ramdisk;
	struct semaphore sem;
	int devNo;
	int device_flag;
};


static int __init mycdrv_init(void)
{
	int err;
	struct asp_mycdrv *my_device;
	int i;
	int result;
	dev_t cdevNo;
	foo_class = class_create(THIS_MODULE, "my_class");
	result = alloc_chrdev_region(&first, 0, NUM_DEVICES,
				"mycdrv");
	if(result < 0){
		printk(KERN_WARNING "Major no : %d not available\n", char_major);
		return result;
	}
	char_major = MAJOR(first);


	for(i = 0; i < NUM_DEVICES; i++){

		my_device = kmalloc(sizeof(struct asp_mycdrv), GFP_KERNEL);
		my_device->devNo = i;
		my_device->device_flag = REGULAR;
		my_device->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
		sema_init(&my_device->sem, 1);
		cdev_init(&my_device->dev, &mycdrv_fops);
		cdevNo = MKDEV(char_major, i);
		err = cdev_add(&my_device->dev, cdevNo, count);
		if(err){
			printk(KERN_NOTICE "\nCould not add the device: %s\n", DEVICE);
		}
		pr_info("\nRegistered device: %s succesfully\n", DEVICE);
		
		device_create(foo_class, NULL, cdevNo, NULL, DEVICE "%d", i);
		pr_info("\n Node created for device no :%d\n",i);
		list_add(&my_device->list, &asp_list);
	}
	return 0;
}

static void __exit my_exit(void) 
{ 
	
	
	int k = 0;
	dev_t cdevNo;
	struct list_head *xlist;	/* pointer to list head object */
	struct list_head *tmp;	/* temporary list head for safe deletion */
	struct asp_mycdrv *my_device;
	if (list_empty(&asp_list)) {
		pr_info("LIST EMPTY! \n");
		return;
	}
	pr_info("LIST NOT empty! \n");
	
	list_for_each_safe(xlist, tmp, &asp_list) {
		cdevNo = MKDEV(char_major, k);
		k++;
		device_destroy(foo_class, cdevNo);
		my_device = list_entry(xlist, struct asp_mycdrv, list);
		cdev_del(&my_device->dev);
		kfree(my_device->ramdisk);
		list_del(&my_device->list);
		pr_info("Device no : %d removed from the linked list \n", my_device->devNo);
		kfree(my_device);
		
	}
	unregister_chrdev_region(first, NUM_DEVICES);
	class_destroy(foo_class);
	if (list_empty(&asp_list))
		pr_info("LIST is empty! \n");
	else
		pr_info("LIST NOT empty! \n");
} 

module_init(mycdrv_init);
module_exit(my_exit);

// Character opening function

static int mycdrv_open(struct inode *inode, struct file *file)
{
	
	struct asp_mycdrv *my_device; /* device information */
	pr_info("\nOpening device : %s\n", DEVICE);
	my_device = container_of(inode->i_cdev, struct asp_mycdrv, dev);
	file->private_data = my_device; /* for other methods */
	
	
	return 0;
}

//Character releasing function
static int mycdrv_release(struct inode *inode, struct file *file)
{
	pr_info("\nClosing device: %s\n", DEVICE);
	return 0;
}


//Character read function

static ssize_t mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos)
{
	int nbytes;
	int k;
	struct asp_mycdrv *my_device = file->private_data;
	if (down_interruptible(&my_device->sem))
		return -ERESTARTSYS;
	if(my_device->device_flag == REGULAR){
		if ((lbuf + *ppos) > ramdisk_size){
			pr_info("trying to read past end of device," "aborting because this is just a stub!\n");
			up(&my_device->sem);
			return 0;
		} 
		nbytes = lbuf - copy_to_user(buf, my_device->ramdisk + *ppos, lbuf); 
		*ppos += nbytes; 
		pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos); 
		up(&my_device->sem);
		return nbytes;
	}
	else{

		if ((lbuf + *ppos) > ramdisk_size){
			pr_info("trying to read past end of device," "aborting because this is just a stub!\n");
			up(&my_device->sem);
			return 0;
		} 
		nbytes = 0;

		for(k = 0; k < lbuf; k++){
			nbytes += 1 - copy_to_user(buf + k, my_device->ramdisk + *ppos, 1);
			*ppos -= 1;
		}
		up(&my_device->sem);
		return nbytes;


	}
	
}


//Character writing function

static ssize_t mycdrv_write(struct file *file, const char __user * buf, size_t lbuf, loff_t * ppos)
{
	int nbytes;
	int k;
	struct asp_mycdrv *my_device = file->private_data;
	if (down_interruptible(&my_device->sem))
		return -ERESTARTSYS;
	if ((lbuf + *ppos) > ramdisk_size) {
			pr_info("trying to read past end of device," "aborting because this is just a stub!\n");
			up(&my_device->sem);
			return 0;
		} 
	if(my_device->device_flag == REGULAR){
		
		nbytes = lbuf - copy_from_user(my_device->ramdisk + *ppos, buf, lbuf);
		*ppos += nbytes; 
		pr_info("\n WRITING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
		up(&my_device->sem);
		return nbytes;
	}

	else{
		
		nbytes = 0;
		for(k = 0; k < lbuf; k++){
			nbytes += 1 - copy_from_user(my_device->ramdisk + *ppos, buf + k, 1);
			*ppos -= 1;
		}
		up(&my_device->sem);
		return nbytes;

	}
}

//Character seeking function

static loff_t mycdrv_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t testpos;
	switch (orig) {
	case SEEK_SET:
		testpos = offset;
		break;
	case SEEK_CUR:
		testpos = file->f_pos + offset;
		break;
	case SEEK_END:
		testpos = ramdisk_size + offset;
		break;
	default:
		return -EINVAL;
	}
	testpos = testpos < ramdisk_size ? testpos : ramdisk_size;
	testpos = testpos >= 0 ? testpos : 0;
	file->f_pos = testpos;
	pr_info("Seeking to pos=%ld\n", (long)testpos);
	return testpos;
}

//Character ioctl function

static long mycdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	
	struct asp_mycdrv *my_device = file->private_data;
	int prev_access_mode = my_device->device_flag;
	int __user *ioargp = (int __user *)arg;
	int rc;
	int dir;
	rc = copy_from_user(&dir, ioargp, sizeof(int));
	if (_IOC_TYPE(cmd) != MYIOC_TYPE) {
		pr_info(" got invalid case, CMD=%d\n", cmd);
		return -EINVAL;
	}
	//pr_info("\nDIRECTION =  %d\n",*dir);
	switch(dir){

		case REVERSE:
			my_device->device_flag = REVERSE;
			return (long)prev_access_mode;
		case REGULAR:
			my_device->device_flag = REGULAR;
			return (long)prev_access_mode;
		default:
			pr_info(" Invalid Case, CMD=%d\n", cmd);
			return -EINVAL;
	}
	


}


MODULE_AUTHOR("Sanjay Nair");
MODULE_DESCRIPTION("CHAR DRIVER");
MODULE_LICENSE("GPL v2");







