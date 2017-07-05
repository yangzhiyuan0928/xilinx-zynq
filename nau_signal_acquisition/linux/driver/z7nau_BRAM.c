#include <linux/module.h>
#include <linux/init.h>		
#include <linux/platform_device.h>

#include <linux/types.h>
#include <linux/of.h>
#include <linux/fs.h>		
#include <asm/io.h>		
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <asm/uaccess.h>
#include <linux/err.h>

#include "z7nau_BRAM.h"




static int z7nau_bram_open(struct inode * inode , struct file * filep)
{
  
  return 0;
}

static int z7nau_bram_release(struct inode * inode, struct file *filep)
{ 
  return 0;
}


static int z7nau_bram_read(struct file *filep, char __user *buffer, size_t length, loff_t *offset)
{
    	
    struct inode *inode = filep->f_inode;
    struct cdev  *chrdev = inode->i_cdev;
    struct z7nau_bram_dev *xdev = container_of(chrdev, struct z7nau_bram_dev, cdev);
     int i, *pdata;  
	if ((xdev->cap == CAP_READ) || (xdev->cap == CAP_R_W))
	  i=copy_to_user((int *)buffer, xdev->base, length*2);
        else
          return  -ENODEV;
	
        pdata = (int *)xdev->base;
        printk("bram data uncopied: %d\n",i);
	return length;
}


static int z7nau_bram_write(struct file *filep,const char __user *buffer, size_t length, loff_t *offset)
{
 
    struct inode *inode = filep->f_inode;
    struct cdev  *chrdev = inode->i_cdev;
    struct z7nau_bram_dev *xdev = container_of(chrdev, struct z7nau_bram_dev, cdev);
	int i=0;

	if ((xdev->cap == CAP_WRITE) || (xdev->cap == CAP_R_W))
	   return copy_from_user(xdev->base, (int *)buffer, length*2);
        else
          return  -ENODEV;	
   

   	
}

static const struct file_operations z7nau_bram_fops =
{
  .owner = THIS_MODULE,
  .open = z7nau_bram_open,
  .release = z7nau_bram_release,
  .read = z7nau_bram_read,
  .write = z7nau_bram_write, 
};

static const struct of_device_id z7nauBRAM_of_match_table[] = {
	{ .compatible = "xlnx,axi-bram-ctrl-4.0",NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, z7nauBRAM_of_match_table);


static int z7nau_bram_probe(struct platform_device *pdev)
{
      struct z7nau_bram_dev *xdev;
      struct resource *mem;
      struct device_node  *node;
      struct device * kdev;
     
      int ret;

	printk("z7nau_bram_probe start!!!\n"); 

        xdev = devm_kzalloc(&pdev->dev, sizeof(*xdev), GFP_KERNEL);
	if (!xdev)
		return -ENOMEM;

        node = pdev->dev.of_node;

	if (!node)
		return -ENODEV;	

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xdev->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(xdev->base))
		return PTR_ERR(xdev->base);


        ret = of_property_read_u8(node,"xlnx,capacity", &xdev->cap);
        ret = of_property_read_string(node,"xlnx,bram-usage",&xdev->bram_name);
        if (ret){
		printk(KERN_WARNING "z7nau_bram: no bram-usage found\n");
		return ret;
	}

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major 
 */
	ret = alloc_chrdev_region(&xdev->devno, Z7NAU_BRAM_MINOR_START, Z7NAU_BRAM_MINOR_COUNT,MODULE_NAME);
	if (ret < 0) {
		printk(KERN_WARNING "z7nau_bram: can't get major \n");
		return ret;
	}
 
        cdev_init(&xdev->cdev, &z7nau_bram_fops);  
	xdev->cdev.owner = THIS_MODULE; 
	xdev->cdev.ops = &z7nau_bram_fops;

        ret = cdev_add(&xdev->cdev, xdev->devno,
		         Z7NAU_BRAM_MINOR_COUNT);
        if (ret){
           dev_err(&pdev->dev,"Err: failed in registering axilite_reg cdev.\n");
	   return ret;
         }
	 
        platform_set_drvdata(pdev, xdev);

        kdev = device_create( z7nau_bram_class,NULL,MKDEV(MAJOR(xdev->devno), Z7NAU_BRAM_MINOR_START),NULL,xdev->bram_name);	
	if(IS_ERR(kdev)){	
		printk("failed to creat device node!\n");
                return PTR_ERR(kdev);
        }
	printk("adc_probe end!!!\n"); 

	return 0;

}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */

static int z7nau_bram_remove(struct platform_device *pdev)
{
struct device *dev = &pdev->dev;
struct z7nau_bram_dev *xdev = platform_get_drvdata(pdev);

  cdev_del (&(xdev->cdev));
  dev_set_drvdata(dev, NULL);
  device_del(&(xdev->dev));

  unregister_chrdev_region(xdev->devno, Z7NAU_BRAM_MINOR_COUNT);
  return 0;
}

static struct platform_driver z7nau_bram_driver = {
	.probe = z7nau_bram_probe,
	.remove = z7nau_bram_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = z7nauBRAM_of_match_table,
	},
};

static int __init z7nau_bram_init(void)
{
	int status;

	/* Register a class that will key udev/mdev to add/remove /dev nodes.
	 *  Last, register the driver which manages those device numbers.
	 */

/* creating class to contain the device for the first time*/
  if(!z7nau_bram_class){
    z7nau_bram_class =class_create(THIS_MODULE, "z7nauBRAM"); 
    if(IS_ERR(z7nau_bram_class)) {
      printk("Err: failed in creating laserCNTRL class.\n");
      return PTR_ERR(z7nau_bram_class) ;
    }	
   }

  status = platform_driver_register(&z7nau_bram_driver);
	if (status < 0) {
		class_destroy(z7nau_bram_class);
	}
	return status;
}
module_init(z7nau_bram_init);

static void __exit z7nau_bram_exit(void)
{
	class_destroy(z7nau_bram_class);
	platform_driver_unregister(&z7nau_bram_driver);

}
module_exit(z7nau_bram_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("seu");
MODULE_DESCRIPTION("xilinx-zynq-bram");





