#include <linux/module.h> //eeclares the init_moule and clenup_module functions
#include <linux/kernel.h> //declares some necessary macros and types and pr_info()
#include <linux/slab.h> //declares the functions related to memory management in kernel space, kmalloc
#include <linux/fs.h> //declares alloc_chrdev_region() funciton
#include <linux/cdev.h>
#include <linux/types.h> 

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LIFO Character Device Driver");
MODULE_AUTHOR("Dakshit Babbar");

#define MAX_LIFO_SIZE 1024*1024
#define MIN(a,b) (a<b)? a: b

//holds the reserved major number for this driver
unsigned int lifo_major;

//stores device specific data
struct lifo_device_data {
    struct cdev cdev;
};

//the two required character devices
struct lifo_device_data ro_dev;
struct lifo_device_data wo_dev;

//the virtual device variables
//the virtual device buffer
char buffer[MAX_LIFO_SIZE];

//one above the top of the stack
unsigned int top;

//define a lock for securing the buffer
static DEFINE_SPINLOCK(dev_lock);

//define a wait queue
static DECLARE_WAIT_QUEUE_HEAD(lifo_dev_wait_queue);

//declare a device nodes
static struct class *lifo_class;
static struct device *lifo_device_ro;
static struct device *lifo_device_wo;

//device driver operations implementation
//open function
static int lifo_open(struct inode *inode, struct file *file){
    pr_info("in lifo_open");
    struct lifo_device_data *lifo_data = container_of(inode->i_cdev, struct lifo_device_data, cdev);

    //validate access to device
    file->private_data = lifo_data;

    //initialize device
    dev_t dev_num = inode->i_rdev; //get the device number from the inode
    unsigned int flags = file->f_flags; //get the flags given by user

    unsigned int minor_num = MINOR(dev_num); 

    if(minor_num==0 && (!!(flags & O_WRONLY) || !!(flags & O_RDWR) || !!(flags & O_APPEND))){
        return -EROFS; //read only file system
    }

    if(minor_num==1 && (!!(flags & O_RDONLY) || !!(flags & O_RDWR))){
        return -EPERM; //permission denied
    }

    pr_info("end lifo_open");
    return 0;
}

//close/release function
static int lifo_release(struct inode *inode, struct file *file){
    return 0;
}

//read function
static ssize_t lifo_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset){
    pr_info("in lifo_read");
    struct lifo_device_data *lifo_data = (struct lifo_device_data *) file->private_data;

    //wait for the writter to write something
    pr_info("wait if the stack is empty");
    bool acquired;
    do{
        wait_event_interruptible(lifo_dev_wait_queue, top != 0);

        spin_lock(&dev_lock);
        acquired=true;
        if(top==0){
            acquired=false;
            spin_unlock(&dev_lock);
        }
    } while(!acquired);

    //length of the data that will be put on the user buffer
    size_t len = MIN(top, size);

    //read the stack down from the top
    pr_info("reading stack from top to bottom starting from top: %u and len: %lu", top, len);
    char local_buffer[len];
    
    int loop_offset=top-1;
    int end=top-len;
    for(int i=loop_offset; i>=end; i--){
        //pr_info("reading pos: %d", i);
        local_buffer[loop_offset-i]=buffer[i];
    }
    //diana@7
    //hello world

    //update the top
    top-=len;

    spin_unlock(&dev_lock);

    pr_info("copying to user_buffer");
    //read data from lifo_data->buffer to user buffer
    if (copy_to_user(user_buffer, local_buffer, len))
        return -EFAULT;

    pr_info("end lifo_read");
    return len;
}

//wite function
static ssize_t lifo_write(struct file *file, const char __user *user_buffer, size_t size, loff_t * offset){
    pr_info("in lifo_write");
    struct lifo_device_data *lifo_data = (struct lifo_device_data *) file->private_data;

    //length of the data that will be put on the user buffer
    spin_lock(&dev_lock);
    size_t len = MIN(MAX_LIFO_SIZE-top, size);

    if (len <= 0)
        return -EPERM; 

    //read data from user buffer to my_data->buffer 
    if (copy_from_user(&buffer[top], user_buffer, len))
        return -EFAULT;

    //update top
    top+=len;

    spin_unlock(&dev_lock);

    wake_up_interruptible(&lifo_dev_wait_queue);

    pr_info("end lifo_write");
    return len;
}

//file operations struct to define alll the necessary operations
const struct file_operations lifo_fops = {
    .owner = THIS_MODULE,
    .open = lifo_open,
    .release = lifo_release,
    .read = lifo_read,
    .write = lifo_write,
};

//initiaise the module
int init_module(){
	int err;
    dev_t device_numbers;

    //Request a range of device numbers from the kernel
    //0 is the starting minor number
    //2 is the count of device minors we want to reserve
    err = alloc_chrdev_region(&device_numbers, 0, 2, "lifodevice");
    if (err < 0) {
        pr_info("Failed to allocate device numbers\n");
        return err;
    }

    lifo_major=MAJOR(device_numbers);
    top=0;
    pr_info("Allocated device numbers: major=%d, minor=%d\n", MAJOR(device_numbers), MINOR(device_numbers));

    //initialization of devices
    cdev_init(&ro_dev.cdev, &lifo_fops);
    cdev_add(&ro_dev.cdev, MKDEV(lifo_major, 0), 1);

    cdev_init(&wo_dev.cdev, &lifo_fops);
    cdev_add(&wo_dev.cdev, MKDEV(lifo_major, 1), 1);

    pr_info("initialization complete");

    //create a device node
    lifo_class = class_create(THIS_MODULE, "lifo_class");
    lifo_device_ro = device_create(lifo_class, NULL, MKDEV(lifo_major, 0), &ro_dev, "lifo_device_ro");
    lifo_device_wo = device_create(lifo_class, NULL, MKDEV(lifo_major, 1), &wo_dev, "lifo_device_wo");

    pr_info("device node made");

	return 0;
}

//ceanup the module+
void cleanup_module(){
    //Destroy the device node
    device_destroy(lifo_class, MKDEV(lifo_major, 0));
    device_destroy(lifo_class, MKDEV(lifo_major, 1));

    //Destroy the class
    class_destroy(lifo_class);

    //delete the devices
    cdev_del(&ro_dev.cdev);
    cdev_del(&wo_dev.cdev);
    
    //unregister the device numbers
    unregister_chrdev_region(MKDEV(lifo_major, 0), 2);

    pr_info("module cleaned up");
}
