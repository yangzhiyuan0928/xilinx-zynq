#include <linux/module.h>
#include <linux/init.h>		
#include <linux/platform_device.h>

#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/fs.h>		
#include <asm/io.h>		
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <asm/uaccess.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>

#include "z7nau_axigpio.h"

wait_queue_head_t wait_key;

static int z7nau_axigpio_open(struct inode *inode, struct file *filep) 
{
 
    struct cdev  *chrdev = inode->i_cdev;
    struct axi_gpio_dev *xdev = container_of(chrdev, struct axi_gpio_dev, cdev);

//enable interrupt
        iowrite32(Z7GPIO_ISR_MASK,xdev->regs+Z7REG_IPISR_OFFSET);
        iowrite32(Z7GPIO_IER_MASK,xdev->regs+Z7REG_IPIER_OFFSET);
        iowrite32(Z7GPIO_GIER,xdev->regs+Z7REG_GIER_OFFSET);
        return 0;
}


static long z7nau_axigpio_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    struct inode *inode = filep->f_inode;
    struct cdev  *chrdev = inode->i_cdev;
    struct axi_gpio_dev *xdev = container_of(chrdev, struct axi_gpio_dev, cdev);
    int value;


    switch (cmd) {
       case Z7AXIGPIO_RDATA:
            value = ioread32(xdev->regs+Z7REG_GPIODATA_OFFSET);
            break; 
       case Z7AXIGPIO_RTRI:
            value = ioread32(xdev->regs+Z7REG_GPIOTRI_OFFSET);
            break;
       case Z7AXIGPIO2_RDATA:
            if (arg == 0)
                value = ioread32(xdev->regs+Z7REG_GPIO2DATA_OFFSET);
            else {
            arg = arg * HZ / 1000;//arg in msecond to system ticks
            wait_event_interruptible_timeout(wait_key, xdev->wait_flag, arg);
            value = ioread32(xdev->regs+Z7REG_GPIO2DATA_OFFSET);
            } 
            break;
       case Z7AXIGPIO2_RTRI:
            value = ioread32(xdev->regs+Z7REG_GPIO2TRI_OFFSET);
            break;
       case Z7AXIGPIO_RGIER:
            value = ioread32(xdev->regs+Z7REG_GIER_OFFSET);
            value = (value >> 31) & Z7GPIO_GIER_MASK;
            break;
       case Z7AXIGPIO_RIER:
            value = ioread32(xdev->regs+Z7REG_IPIER_OFFSET);
            value = (value & Z7GPIO_IER_MASK) >> 1;
            break;
       case Z7AXIGPIO_RISR:
            value = ioread32(xdev->regs+Z7REG_IPISR_OFFSET);
            value = (value & Z7GPIO_ISR_MASK) >> 1;
            break; 
       case Z7AXIGPIO_WDATA:
            iowrite32(arg,xdev->regs+Z7REG_GPIODATA_OFFSET);
            break; 
       case Z7AXIGPIO_WTRI:
            iowrite32(arg,xdev->regs+Z7REG_GPIOTRI_OFFSET);
            break;
       case Z7AXIGPIO2_WTRI:
            iowrite32(arg,xdev->regs+Z7REG_GPIO2TRI_OFFSET);
            break;
       case Z7AXIGPIO_WGIER:
            arg = arg << 31;
            iowrite32(arg,xdev->regs+Z7REG_GIER_OFFSET);
            break;
       case Z7AXIGPIO_WIER:
            arg =( arg << 1 ) & Z7GPIO_IER_MASK;
            iowrite32(arg,xdev->regs+Z7REG_IPIER_OFFSET);
            break;
       default:
            return -EOPNOTSUPP;

    }

   	return value;
}

static int z7nau_axigpio_release(struct inode * inode, struct file *filep)
{ 

    struct cdev  *chrdev = inode->i_cdev;
    struct axi_gpio_dev *xdev = container_of(chrdev, struct axi_gpio_dev, cdev);

        iowrite32(0xffffffff,xdev->regs+Z7REG_IPISR_OFFSET);
        iowrite32(0x00000000,xdev->regs+Z7REG_IPIER_OFFSET);
        iowrite32(0x00000000,xdev->regs+Z7REG_GIER_OFFSET);
  return 0;
}



static const struct file_operations axigpio_fileops = {
	//.read = z7nau_axigpio_read,     
	//.write = z7nau_axigpio_write, 
	.unlocked_ioctl = z7nau_axigpio_ioctl,
	.release = z7nau_axigpio_release,
	.open = z7nau_axigpio_open,
	.owner = THIS_MODULE,
};

static irqreturn_t axigpio_interrupt_handler(int irq, void *devid)
{
  struct axi_gpio_dev *xdev = devid;	
  
  printk("interrupt handler function! irq = %d\n",irq);

   //clear int flag
   iowrite32(0xffffffff,xdev->regs+Z7REG_IPISR_OFFSET);

   if(waitqueue_active(&wait_key)){
      xdev->wait_flag = 1;
      wake_up_interruptible( &wait_key);
   }
 
    return IRQ_HANDLED;
}



static const struct of_device_id z7nau_axigpio_of_match_table[] = {  
	{ .compatible = "xlnx,z7nau-axi-GPIO-1.0", NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, z7nau_axigpio_of_match_table); 



static int z7nau_axigpio_probe(struct platform_device *pdev)
{
        struct axi_gpio_dev *xdev;
        struct device_node  *node;
        struct resource *res;
        struct device *devp;
        int    status,value;
	
 
	printk("axi_gpio_probe start!!!\n"); 

        xdev = devm_kzalloc(&pdev->dev, sizeof(*xdev), GFP_KERNEL);
	if (!xdev)
		return -ENOMEM;

	xdev->dev = &(pdev->dev);
       node = pdev->dev.of_node;

	if (!node)
		return -ENODEV;
	/* Map the registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xdev->regs = devm_ioremap_resource(&pdev->dev, res);

        /*************Hardware initialization*******************/
        of_property_read_u32(node, "xlnx,is-dual", &value);
        if (value ==0){
           printk("axigpio isn't dual port mode!!!\n");
           return -EINVAL;
         }
       
        of_property_read_u32(node, "xlnx,all-outputs", &value);
        if (value ==0){
           printk("axigpio port1 isn't all output mode!!!\n");
           return -EINVAL;
         }

        of_property_read_u32(node, "xlnx,all-inputs-2", &value);
        if (value ==0){
           printk("axigpio port2 isn't all input mode!!!\n");
           return -EINVAL;
         }

        of_property_read_u32(node, "xlnx,gpio2-width", &value);
        if (value != 1){
           printk("Don't support axigpio port2 width more than 1!!!\n");
           return -EINVAL;
         }

        of_property_read_u32(node, "xlnx,interrupt-present", &value);
        if (value != 1){
           printk("no axigpio interrupt defined!!\n");
           return -EINVAL;
         }

        of_property_read_u32(node, "xlnx,dout-default", &value);
        iowrite32(value,xdev->regs+Z7REG_GPIODATA_OFFSET);

   
        iowrite32(Z7GPIO_DIR,xdev->regs+Z7REG_GPIOTRI_OFFSET);
        iowrite32(Z7GPIO2_DIR,xdev->regs+Z7REG_GPIO2TRI_OFFSET);

        iowrite32(0xffffffff,xdev->regs+Z7REG_IPISR_OFFSET);
        iowrite32(0x00000000,xdev->regs+Z7REG_IPIER_OFFSET);
        iowrite32(0x00000000,xdev->regs+Z7REG_GIER_OFFSET);


	/* find the IRQ line, if it exists in the device tree */
	xdev->irq = irq_of_parse_and_map(node, 0);
	status = request_irq(xdev->irq, axigpio_interrupt_handler,
			  IRQF_TRIGGER_HIGH | IRQF_SHARED, MODULE_NAME, xdev);
	if (status) {
		dev_err(xdev->dev, "axi GPIO unable to request IRQ %d\n", xdev->irq);
		return status;
	}

       status =alloc_chrdev_region(&xdev->devno, 
		AXIGPIO_MINOR_START, AXIGPIO_MINOR_COUNT,MODULE_NAME);
       if (status < 0) {
	      dev_err(&pdev->dev, "unable to alloc axi GPIO chrdev \n");
	      return status;
       }
	

       cdev_init(&xdev->cdev, &axigpio_fileops);  
	xdev->cdev.owner = THIS_MODULE; 
	xdev->cdev.ops = &axigpio_fileops;

        status = cdev_add(&xdev->cdev, xdev->devno,
		         AXIGPIO_MINOR_COUNT);
        if (status){
           dev_err(&pdev->dev,"Err: failed in registering axigpio cdev.\n");
	   return status;
         }
	 
        platform_set_drvdata(pdev, xdev);

        axigpio_class = class_create(THIS_MODULE,MODULE_NAME); 
        if(IS_ERR(axigpio_class))
        {
	   printk("failed creat  axigpio_class !\n");
        }
        devp = device_create( axigpio_class,NULL,xdev->devno,NULL,MODULE_NAME);	
	if(IS_ERR(devp))	
		printk("failed to creat device node!\n");

        xdev->wait_flag = 0;
        init_waitqueue_head (&wait_key);  
	printk("axigpio_probe end!!!\n"); 

	return SUCCESS;
	
}

static int z7nau_axigpio_remove(struct platform_device *pdev)
{
      	
     struct axi_gpio_dev *xdev = platform_get_drvdata(pdev);

	if (xdev->irq > 0)
		free_irq(xdev->irq, xdev);

        cdev_del(&xdev->cdev);
        device_destroy(axigpio_class,MKDEV(MAJOR(xdev->devno),
		AXIGPIO_MINOR_START));
        unregister_chrdev_region(xdev->devno, AXIGPIO_MINOR_COUNT);

  	class_destroy(axigpio_class);

	return SUCCESS;
}

static struct platform_driver z7nau_axigpio_driver = {     
	.probe  = z7nau_axigpio_probe,    
	.remove = z7nau_axigpio_remove,
	.driver = {
		.name = MODULE_NAME,     
		.owner = THIS_MODULE,
		.of_match_table =z7nau_axigpio_of_match_table,
	},
};
module_platform_driver(z7nau_axigpio_driver);



MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("seu");
MODULE_DESCRIPTION("axi GPIO and key interrupt driver");
MODULE_VERSION("1.00");
