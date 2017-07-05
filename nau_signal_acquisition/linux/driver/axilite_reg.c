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

#include "axilite_reg.h"

int axilite_reg_fasync(int fd,struct file *filep,int mode)  
{
    struct inode *inode = filep->f_inode;
    struct cdev  *chrdev = inode->i_cdev;
    struct axilite_reg_dev *xdev = container_of(chrdev, struct axilite_reg_dev, cdev);
    int retval;

    printk("driver test_fasync fuction, fasync_helper fuction\n");

    retval=fasync_helper(fd,filep,mode,&xdev->async_queue);  //初始化/释放fasync_struct结构体
                                                       
    if(retval<0) {                                             
       printk( "the retval is %d !\n",retval);                   
    return retval;                                       
 
    }
    else  printk( "the retval is %d !\n",retval);
    return 0 ;
}


static int axilite_reg_open(struct inode *inode, struct file *filp) 
{
    return 0;
}


static long axilite_reg_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    struct inode *inode = filep->f_inode;
    struct cdev  *chrdev = inode->i_cdev;
    struct axilite_reg_dev *xdev = container_of(chrdev, struct axilite_reg_dev, cdev);
    int value = 0;


    switch (cmd) {  //对应到AXI-Lite总线读写寄存器
       case AXILITE_RDALENGTH:
            value = ioread32(xdev->regs+DA_LENGTH_REG_OFFSET);
            break; 
       case AXILITE_RADLENGTH:
            value = ioread32(xdev->regs+AD_LENGTH_REG_OFFSET);
            break;
       case AXILITE_RDARSTN:
            value = ioread32(xdev->regs+RSTN_REG_OFFSET);
            value = (value & DA_RSTN_mask) >> 1; 
            break;
       case AXILITE_RADRSTN:
            value = ioread32(xdev->regs+RSTN_REG_OFFSET);
            value = (value & AD_RSTN_mask); 
            break;
       case AXILITE_RTRIGSEL:
            value = ioread32(xdev->regs+TRIG_REG_OFFSET);
            value = (value & TRIG_SEL_mask) >> 1; 
            break;
       case AXILITE_RTRIGINT:
            value = ioread32(xdev->regs+TRIG_REG_OFFSET);
            value = (value & TRIG_INT_mask); 
            break;
       case AXILITE_RLEDON_TIME:
            value = ioread32(xdev->regs+LEDON_TIME_REG_OFFSET);
            break;
       case AXILITE_RLEDOFF_TIME:
            value = ioread32(xdev->regs+LEDOFF_TIME_REG_OFFSET);
            break;
       case AXILITE_WDALENGTH:
            iowrite32(arg,xdev->regs+DA_LENGTH_REG_OFFSET);
            break; 
       case AXILITE_WADLENGTH:
            iowrite32(arg,xdev->regs+AD_LENGTH_REG_OFFSET); 
            break;
       case AXILITE_WDARSTN:
            value = ioread32(xdev->regs+RSTN_REG_OFFSET);
            value = value & ~DA_RSTN_mask ;
            value = value | ((arg << 1) & DA_RSTN_mask);
            iowrite32(value,xdev->regs+RSTN_REG_OFFSET);
            break;
       case AXILITE_WADRSTN:
            value = ioread32(xdev->regs+RSTN_REG_OFFSET);
            value = value & ~AD_RSTN_mask ;
            value = value | (arg & AD_RSTN_mask);
            iowrite32(value,xdev->regs+RSTN_REG_OFFSET);
            break;
       case AXILITE_WTRIGSEL:
            value = ioread32(xdev->regs+TRIG_REG_OFFSET);
            value = value & ~TRIG_SEL_mask ;
            value = value | ((arg << 1) & TRIG_SEL_mask);
            iowrite32(value,xdev->regs+TRIG_REG_OFFSET);
            break;
       case AXILITE_WTRIGINT:
            value = ioread32(xdev->regs+TRIG_REG_OFFSET);
            value = value & ~TRIG_INT_mask ;
            value = value | (arg  & TRIG_INT_mask);
            iowrite32(value,xdev->regs+TRIG_REG_OFFSET);
            break;
       case AXILITE_WLEDON_TIME:
            iowrite32(arg,xdev->regs+LEDON_TIME_REG_OFFSET);
            break;
       case AXILITE_WLEDOFF_TIME:
            iowrite32(arg,xdev->regs+LEDOFF_TIME_REG_OFFSET);
            break;
       default:
            return -EOPNOTSUPP;
    }

   	return value;
}

static int axilite_reg_release(struct inode * inode, struct file *filep)
{ 
  axilite_reg_fasync(-1,filep,0);  //释放fasync_stryct
  return 0;
}

static const struct file_operations axilite_reg_fileops = {
	//.read = axilite_reg_read,     
	//.write = adc_write, 
	.unlocked_ioctl = axilite_reg_ioctl,
	.release = axilite_reg_release,  //当打开最后一个设备的用户进程执行close后，内核将调用驱动该函数，清理未完成的输入输出操作，释放资源等
	.open = axilite_reg_open,
	.owner = THIS_MODULE,
	.fasync = axilite_reg_fasync,
};

static irqreturn_t axilite_reg_interrupt_handler(int irq, void *devid)
{
  struct axilite_reg_dev *xdev = devid;	
  
  printk("interrupt handler function! irq = %d\n",irq);
  if(xdev->async_queue)  
  {
       	kill_fasync(&xdev->async_queue,SIGIO,POLL_IN);  //发送信号SIGIO给fasync_struct结构体所描述PID，触发应用层序的SIGIO信号处理函数
  }
  else printk( "kill_fasync failed !\n");


    return IRQ_HANDLED;
}

static const struct of_device_id axilite_reg_of_match_table[] = {  
	{ .compatible = "xlnx,z7nau-axi-lite-1.0", NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, axilite_reg_of_match_table); 

static int axilite_reg_probe(struct platform_device *pdev)
{
	struct axilite_reg_dev *xdev;
	struct device_node  *node;
	struct resource *res;
	struct device  *devp;
	int    status;


	printk("axilite_reg_probe start!!!\n"); 

	xdev = devm_kzalloc(&pdev->dev, sizeof(*xdev), GFP_KERNEL);  //和设备相关的内核内存分配
	if (!xdev)
		return -ENOMEM;

	xdev->dev = &(pdev->dev);
	node = pdev->dev.of_node;

	if (!node)
		return -ENODEV;
	
	/* Map the registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	xdev->regs = devm_ioremap_resource(&pdev->dev, res);  //IO内存映射


	/* find the IRQ line, if it exists in the device tree */
	xdev->irq = irq_of_parse_and_map(node, 0);  //设备树里查找中断的描述项，然后返回中断号
	status = request_irq(xdev->irq, axilite_reg_interrupt_handler,
		  IRQF_TRIGGER_RISING | IRQF_SHARED, MODULE_NAME, xdev);  
	if (status) {
		dev_err(xdev->dev, "unable to request IRQ %d\n", xdev->irq);
		return status;
	}

	status = alloc_chrdev_region(&xdev->devno, 
	AXILITE_REG_MINOR_START, AXILITE_REG_MINOR_COUNT,MODULE_NAME);  //申请设备号
	if (status < 0) {
		dev_err(&pdev->dev, "unable to alloc chrdev \n");
		return status;
	}


	cdev_init(&xdev->cdev, &axilite_reg_fileops);   //设备初始化 
	xdev->cdev.owner = THIS_MODULE; 
	xdev->cdev.ops = &axilite_reg_fileops;

	status = cdev_add(&xdev->cdev, xdev->devno,  //添加设备
	         AXILITE_REG_MINOR_COUNT);
	if (status){
	   dev_err(&pdev->dev,"Err: failed in registering axilite_reg cdev.\n");
	return status;
	 }

	platform_set_drvdata(pdev, xdev);

	axilite_reg_class = class_create(THIS_MODULE,MODULE_NAME); 
	if(IS_ERR(axilite_reg_class))
	{
		printk("failed creat  adc_class !\n");
	}
	devp = device_create( axilite_reg_class,NULL,xdev->devno,NULL,MODULE_NAME);	 //创建设备节点
	if(IS_ERR(devp))	
		printk("failed to creat device node!\n");

	printk("axilite_reg probe end!!!\n"); 

	return SUCCESS;
	
}

static int axilite_reg_remove(struct platform_device *pdev)
{      	
	struct axilite_reg_dev *xdev = platform_get_drvdata(pdev);

	if (xdev->irq > 0)
		free_irq(xdev->irq, xdev);

	cdev_del(&xdev->cdev);
	device_destroy(axilite_reg_class,MKDEV(MAJOR(xdev->devno), AXILITE_REG_MINOR_START));
	unregister_chrdev_region(xdev->devno, AXILITE_REG_MINOR_COUNT);  //释放设备号

	class_destroy(axilite_reg_class);

	return SUCCESS;
}

static struct platform_driver axilite_reg_driver = {     
	.probe = axilite_reg_probe,    
	.remove = axilite_reg_remove,
	.driver = {
		.name = MODULE_NAME,     
		.owner = THIS_MODULE,
		.of_match_table = axilite_reg_of_match_table,  //设备描述，设备树文件匹配
	},
};
module_platform_driver(axilite_reg_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("seu");
MODULE_DESCRIPTION("axilite register and adc interrupt driver");
MODULE_VERSION("1.00");
