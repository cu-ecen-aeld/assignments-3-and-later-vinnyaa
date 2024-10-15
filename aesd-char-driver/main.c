/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
#include <linux/slab.h>
#include <linux/mutex.h>
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Vincent Anderson"); /** DONE: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *sample_aesd_dev;
    PDEBUG("open");
    /**
     * DONE: handle open
     */
    sample_aesd_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = sample_aesd_dev;
    
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * DONE: handle release
     * nothing to do here?
     */
    
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    size_t read_offset;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * DONE: handle read
     */
     
    // convert filp to aesd_dev structure
    struct aesd_dev *dev = filp->private_data;

    //initialize temporary buffer
    struct aesd_buffer_entry *temp_entry;
        
    //mutex steps
    if(mutex_lock_interruptible(&dev->lock)){
        return -ERESTARTSYS;
    }
    
    //reading buffer from user space to kernal space
    temp_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circ_buffer, *f_pos, &read_offset);
    if(temp_entry) {
	size_t to_copy = min(count, temp_entry->size - read_offset);
	if(copy_to_user(buf, temp_entry->buffptr + read_offset, to_copy)) {
	    retval = -EFAULT;
	    mutex_unlock(&dev->lock);
	    return retval;
	}
	retval = to_copy;
	*f_pos += to_copy;    	       
   }
    
 
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    //char *unadded_buff_ptr;
    struct aesd_buffer_entry resized_entry;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    
    // mutex lock 
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }
    
    // if no entry has been made to the circle buffer, try to allocate memory
    // if not enough memory available, set error as -ENOMEM
    if(dev->temp_entry.size == 0) {
    	dev->temp_entry.buffptr = kmalloc(count, GFP_KERNEL);
    	if(dev->temp_entry.buffptr == NULL) {
    	    retval = -ENOMEM;
    	    mutex_unlock(&dev->lock);
    	    return retval;
    	}
    	dev->temp_entry.size = 0;
    } else {
    	// place holder element to try to insert another element to aesd_circlebuffer
    	char *temp_buff_pointer = krealloc(dev->temp_entry.buffptr, dev->temp_entry.size + count, GFP_KERNEL);
    	if(temp_buff_pointer == NULL) {
    	    retval = -ENOMEM;
    	    mutex_unlock(&dev->lock);
    	    return retval;
    	}
    	dev->temp_entry.buffptr = temp_buff_pointer;
    }
    
    // actually copy from user space into kernal mode
    if(copy_from_user((void *)(dev->temp_entry.buffptr + dev->temp_entry.size), buf, count)) {
        retval = -EFAULT;
        mutex_unlock(&dev->lock);
        return retval;
    }
    
    // update buffer element size, since if statement did not trigger, copy_from_user completed
    dev->temp_entry.size += count;
    
    // data is now in kernel space, now we can move it to the aesd circle buffer

    // can I find a newline character within the buffer entry?
    if(dev->temp_entry.buffptr[dev->temp_entry.size-1] == '\n') {
        // new location for the end of the buffer text entry should be considered
        resized_entry.buffptr = kmalloc(dev->temp_entry.size, GFP_KERNEL);

        if(!resized_entry.buffptr) {
            retval = -ENOMEM;
            mutex_unlock(&dev->lock);
            return retval;
        }

        // kmalloc success, i can move the data to the new temp place
        memcpy((void *)resized_entry.buffptr, dev->temp_entry.buffptr, dev->temp_entry.size);
        resized_entry.size = dev->temp_entry.size;

        //unadded_buff_ptr = 
        aesd_circular_buffer_add_entry(&dev->circ_buffer, &resized_entry);


        // if a valid pointer is returned, the entry will overwrite an entry added.
        // Deallocate memory associated with the overwritten command.
        //if(unadded_buff_ptr) {
         //   kfree(unadded_buff_ptr);
        //}

        // temp entry is created, now replace temp_entry buffer pointer and free previous memory
        dev->temp_entry.size = 0;
        kfree(dev->temp_entry.buffptr);
        dev->temp_entry.buffptr = NULL;
    }
    
    retval = count;
    
    // mutex unlock
    mutex_unlock(&dev->lock);
    return retval;
}


//assigment 9 additions

static loff_t aesd_llseek(struct file *filp, loff_t off, int whence) {
    struct aesd_dev *dev = filp->private_data;
    loff tmp_pos = 0;
    loff_t buff_size;
    size_t buff_offset;
    size_t index;
    
    if(mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }
    
    buff_size = 0;
    for(index = 0; i< AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; index++) {
        buff_size += dev->circ_buffer.temp_entry[index].size;
    }
    
    
    switch (whence) {
        case SEEK_SET:
            tmp_pos = off;
            break;
        
        case SEEK_CUR:
            tmp_pos = filp->fpos + off;
            break;
        
        case SEEK_END:
            tmp_pos = buff_size + off;
            break;
        
        default:
            mutex_unlock(&dev->lock);
            return -ERESTARTSYS;
    }
    
    if (tmp_pos < 0 || tmp_pos > buff_size) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    
    filp->fpos = tmp_pos;
    mutex_unlock(&dev->lock);
    return tmp_pos;

}

//assignment 9 additions
long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    size_t offset;
    loff_t tmp_pos = 0;
    loff_t buff_pos = 0;
    int retval = 0;
    int index = 0;
    
    
    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC || _IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) {
        // cmd is not matching aesd value of 16, dont continue further
        return -ENOTTY;
    
    }   
    
    // mutex must lock to continue
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }
    
    switch (cmd) {
        case AESDCHAR_IOCSEEKTO:
            if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto))) {
                mutex_unlock(&dev->lock);
                return -EFAULT;
            }
            
            if (seekto.write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED || dev->circ_buffer.temp_entry[seekto.write_cmd].size == 0) {
                mutex_unlock(&dev->lock);
                return -EINVAL;
            }
            
            if (seekto.write_cmd_offset >= dev->circ_buffer.temp_entry[seekto.write_cmd].size) {
                mutex_unlock(&dev->lock);
                return -EINVAL;
            }
            
            for (index = 0; index < seekto.write_cmd; index++) {
                buff_pos += dev->circular_buffer.temp_entry[index].size;
            }
            temp_pos = buff_pos + seekto.write_cmd_offset;
            
            filp->fpos = tmp_pos;
            retval = tmp_pos;
            break;
            
        default:
            mutex_unlock(&dev->lock);
            return -EINVAL;
    }
    
    
}    



struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_unlocked_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * DONE: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.circ_buffer);


    mutex_init(&aesd_device.lock);


    result = aesd_setup_cdev(&aesd_device);
    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    struct aesd_buffer_entry *it_entry;
    size_t index;
    cdev_del(&aesd_device.cdev);

    /**
     * DONE: cleanup AESD specific poritions here as necessary
     */

    AESD_CIRCULAR_BUFFER_FOREACH(it_entry, &aesd_device.circ_buffer, index) {
        kfree(it_entry->buffptr);
    }

    unregister_chrdev_region(devno, 1);
    mutex_destroy(&aesd_device.lock);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
