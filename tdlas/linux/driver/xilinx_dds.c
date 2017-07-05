/*********************************************************
File Name:    DDS_dev.c
Author:       yangzhiyuan
Version:      v0.1 2016-6-10
Description:  Driver of dds_dev peripheral.
*********************************************************/
#include <linux/module.h>
#include <linux/init.h>		//insmod
#include <linux/platform_device.h>

#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/fs.h>   //struct file_operations
#include <asm/io.h>		  //memory map
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/device.h>

#define DEVICE_NAME         "xilinx_dds" 

#define DDS_PHY_ADDR    0x69000000 	//Modify the address to your peripheral
#define DDS_REG_NUM     4
#define MY_DDS_REG_WIDTH    32

static void __iomem *DDS_Regs;

static int xlinx_dds_open(struct inode * inode , struct file * filp)
{
  printk("xilinx-dds: opened.\n");
  return 0;
}

static int xilinx_dds_release(struct inode * inode, struct file *filp)
{ 
	printk("xilinx-dds: released.\n");
  return 0;
}

static int xilinx_dds_read(struct file *filp, int *buffer, size_t length, loff_t * offset)
{
     *buffer = ioread32(DDS_Regs+(length*4) );
     return 0;
}

static int xilinx_dds_ioctl(struct file *filp, unsigned int reg_num, unsigned long arg)
{
   iowrite32(arg, DDS_Regs+reg_num*4);
   return 0;
}

static const struct file_operations dds_fops =
{
  .owner          = THIS_MODULE,
  .open           = xlinx_dds_open,
  .release        = xilinx_dds_release,
  .read           = xilinx_dds_read,
  .unlocked_ioctl = xilinx_dds_ioctl, 
};

static struct miscdevice dds_dev =
{
  .minor = MISC_DYNAMIC_MINOR,
  .name  = DEVICE_NAME,
  .fops  = &dds_fops,
};

static int __init dds_init(void)
{
  int ret, val;

  ret = misc_register(&dds_dev);
	if(ret)  {
	 printk("dds_dev:[errror] Misc device register failed.\n");
	 return ret;
	}
  //Map device to the virtual address of kernel 
  DDS_Regs = ioremap(DDS_PHY_ADDR, DDS_REG_NUM); /* Verify it's non-null! */
  printk("xilinx-dds: Access address to device is:0x%x\n", (unsigned int)DDS_Regs);
  if(DDS_Regs == NULL) {
     printk("xilinx-dds:[ERROR] Access address is NULL!\n");
     return -EIO;
  }   
  printk("xilinx-dds: xilinx-dds module init success.\n");
  //val = ioread32(AXI_AD_Regs+0xC);
 	return ret;
}

static void __exit dds_exit(void)
{
  iounmap(DDS_Regs);
  //release_mem_region(AXI_AD_Regs, AXI_AD_REG_NUM*MY_GPIO_REG_WIDTH);
  misc_deregister(&dds_dev);    //unregister device
  printk("xilinx-dds: Module exit\n");
}

module_init(dds_init);
module_exit(dds_exit);

MODULE_AUTHOR("seu");
MODULE_ALIAS("xilinx-dds");
MODULE_DESCRIPTION("xilinx-dds module");
MODULE_LICENSE("GPL");
