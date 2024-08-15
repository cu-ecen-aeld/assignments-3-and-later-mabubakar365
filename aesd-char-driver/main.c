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
#include<linux/kernel.h>
#include <linux/slab.h>
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Muhammad Abu Bakar"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    struct aesd_dev *dev = filp->private_data;
    size_t entry_offset;
    struct aesd_buffer_entry *entry;

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &entry_offset);
    if(entry == NULL)
    {
        retval = 0;
        goto clean;
    }

    if(entry != NULL)
    {
        size_t available_bytes = entry->size - entry_offset;
        if(available_bytes < count)
        {
            count = available_bytes;
        }

        if(copy_to_user(buf, entry->buffptr + entry_offset, count))
        {
            retval = -EFAULT;
            PDEBUG("Error in copy to user function");
            goto clean;
        }

        else
        {
            *f_pos += count;
            retval = count;
        }
    }

    clean:
        return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    /* initialize the return value */
    ssize_t retval = -ENOMEM;

    /* check if filp pointer is valid */
    if(filp == NULL) return -EINVAL;
    /* check if user buffer have data */
    if(buf == NULL) return -EINVAL;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    struct aesd_dev *dev = filp->private_data;
    /* acquire mutex lock */
    if(mutex_lock_interruptible(&dev->lock))
    {
        PDEBUG("Error in Mutex Locking");
        retval = -ERESTARTSYS;
        goto clean;
    }

    /* check if there is vlaid data count */
    if(count <= 0)
    {
        retval = -EINVAL;
        goto clean;
    }

    /* declare a buffer_tmp pointer to hold the address of buffer where data would be written and handed over to circular buffer */
    char *buffer_tmp = NULL;
    /* If cache buffer is NULL it's mean we are looking for data of new entry so allocate space for that */
    if(dev->entry_cache.buffptr == NULL)
    {
        buffer_tmp = kmalloc(count, GFP_KERNEL);
    }
    /* if cache buffer is not NULL it's mean we are looking for remaining data terminated with \n hence increase the existing space of buffer */
    else
    {
        buffer_tmp = krealloc(dev->entry_cache.buffptr, dev->entry_cache.size+count, GFP_KERNEL);
    }

    /* check if space is allocated */
    if(buffer_tmp == NULL)
    {
        PDEBUG("Unable to allocate space for the buffer");
        goto clean;
    }

    /* copy data from user space to kernel space 
       update the data write address in case of krealloc
       In case of malloc dev->entry_cache.size would be = 0
       In case of krealloc dev->entry_cache.size would be > 0
    */
    if(copy_from_user(buffer_tmp+dev->entry_cache.size, buf, count))
    {
        retval = -EFAULT;
        kfree(buffer_tmp);
        goto clean;
    }

    else
    {
        retval = count;
    }

    /* update the cache entry with address of buffer as well as size of data */
    dev->entry_cache.buffptr = buffer_tmp;
    dev->entry_cache.size = dev->entry_cache.size + count;

    /* Check if last byte of buffer is newline character */
    const char *free_buffer = NULL;
    if(dev->entry_cache.buffptr[dev->entry_cache.size-1] == '\n')
    {
        /* add data to circular buffer */
        free_buffer = aesd_circular_buffer_add_entry(&dev->circular_buffer, &dev->entry_cache);
        /* check if buffer is full */
        if(free_buffer)
        {
            /* free the oldest buffer */
            kfree(free_buffer);
            /* update the size information */
            dev->buffer_size -= dev->circular_buffer.entry[dev->circular_buffer.in_offs].size;
        }

        /* update the size information */
        dev->buffer_size += dev->entry_cache.size;
        /* Data written to circular buffer so clear the cache entry */
        dev->entry_cache.buffptr = NULL;
        dev->entry_cache.size = 0;
    }

    retval = count;
    *f_pos = count;

    clean:
        mutex_unlock(&dev->lock);
        return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence)
    {
        case SEEK_SET:
            newpos =  off;
            break;
        case SEEK_CUR:
            newpos = filp->f_pos + off;
            break;
        case SEEK_END:
            newpos = dev->buffer_size + off;
            break;
        default:
            return -EINVAL;
    }

    if(newpos < 0)
    {
        return -EINVAL;
    }

    filp->f_pos = newpos;
    return newpos;
}

// loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
// {
//     struct aesd_dev *dev = filp->private_data;
//     loff_t retval;
//     size_t buffsize = 0;

//     if(mutex_lock_interruptible(&dev->lock))
//     {
//         PDEBUG("Error in Mutex locking");
//         retval = -ERESTARTSYS;
//         goto clean;
//     }

    // printk(KERN_INFO "dev->buffer_size : %d\n", dev->buffer_size);
    // buffsize = aesd_circular_buffer_size(&dev->circular_buffer);
    // printk(KERN_INFO "buffsize: %d\n", buffsize);
    // retval = fixed_size_llseek(filp, off, whence, dev->buffer_size);

//     clean:
//         mutex_unlock(&dev->lock);
//         return retval;
// }

long aesd_adjust_file_offset(struct file *filp, uint32_t write_cmd, uint32_t write_cmd_offset)
{
    long retval = 0;
    struct aesd_dev *dev = filp->private_data;
    long buffer_offset = 0;

    if(mutex_lock_interruptible(&dev->lock))
    {
        PDEBUG("Error in Mutex locking");
        retval = -ERESTARTSYS;
        goto clean;
    }

    if(write_cmd >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED || write_cmd_offset > dev->circular_buffer.entry[write_cmd].size)
    {
        retval = -EINVAL;
        goto clean;
    }

    for(int i = 0; i < write_cmd; i++)
    {
        buffer_offset += dev->circular_buffer.entry[i].size;
    }

    filp->f_pos = buffer_offset + write_cmd_offset;

    clean:
        mutex_unlock(&dev->lock);
        return retval;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;

    long retval;

    if(_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
    if(_IOC_NR(cmd) != AESDCHAR_IOC_MAXNR) return -ENOTTY;

    switch(cmd)
    {
        case AESDCHAR_IOCSEEKTO:
            if(copy_from_user(&seekto, (struct aesd_seekto __user *)arg, sizeof(seekto)))
            {
                return -EFAULT;
            }

            else
            {
                printk(KERN_INFO "Write command: %d", seekto.write_cmd);
                printk(KERN_INFO "Write command Offset: %d", seekto.write_cmd_offset);
                retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);
            }

            break;

        default:
            return -ENOTTY;
    }

    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl
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
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    aesd_device.entry_cache.buffptr = NULL;
    aesd_device.entry_cache.size = 0;
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

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    struct aesd_buffer_entry *entry;
    uint8_t index;

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index)
    {
        kfree(entry->buffptr);
    }
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
