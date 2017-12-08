/*
 * AXI ADC DMA driver for ad7980.
 * Configures ADC in continuous stream and DMA in cyclic mode.
 * Uses DMA Client API to configure Xilinx DMA engine. 
 * Provide interface to read ADC Samples.
 * Copyright (C) 2012 Xilinx, Inc.
 * Copyright (C) 2012 Robert Armstrong
 *
 * Author: Radhey Shyam Pandey <radheys@xilinx.com>
 * 
 * Based on PL330 DMA Driver.
 * For using two DMA controllers, each has one adc channel IP connected.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <asm/uaccess.h>
#include <linux/dma/xilinx_dma.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include "axidma_ADC.h"


struct class *axidma_adc_class;

/*ADC channel name */ 
/*
static const char adc_channels[][20]= {
    {"channel1x"},{"channel2x"}
};
*/

#define XILINX_DMA_NUM_APP_WORDS	5
#define XILINX_DMA_MAX_CHANS_PER_DEVICE	0x20

#define ADC_START_REG_OFFSET            4

#define ADSTART_ON                      0x1
#define ADSTART_OFF                     0x0


/* Function Declarations */
static int axi_adc_dma_init(struct axi_dma_adc_dev *adc_dev);
static int axi_adc_dma_config(struct axi_dma_adc_dev *adc_dev);
static void axi_adc_slave_rx_callback(void *completion);


/* Pre DMA initialization . Request handle to Xilinx DMA engine
*/
static int axi_adc_dma_init(struct axi_dma_adc_dev *adc_dev)
{
    int status = FAILURE;

    /* slave transfer channels */

   adc_dev->rx_chan = dma_request_slave_channel(&adc_dev->pdev->dev, axidma_adc_dev->chan_name);
 printk(KERN_EMERG "dma_init: adc_dev->rx_chan = %x \n",(unsigned int)adc_dev->rx_chan);
   if (!adc_dev->rx_chan) {
       pr_err("xilinx_dma: No Rx channel\n");
      return -ENODEV;
   }
    status = axi_adc_dma_config(adc_dev);
    if (status) {
	dma_release_channel(adc_dev->rx_chan);
        pr_err("chan config failed!\n");
    }
    return status;
}

/*Configures buffer/flags,timeouts for DMA */
static int axi_adc_dma_config(struct axi_dma_adc_dev *adc_dev)
{
    int ret = SUCCESS;
    int buf_size = AXI_ADC_BUFF_SIZE * AXI_ADC_BUFF_NUM;
//    struct dma_device *rx_dev = NULL;

    /* Allocate BD descriptors buffers */
    adc_dev->dsts = kzalloc(buf_size, GFP_KERNEL);
    if (!adc_dev->dsts)
	goto err_srcbuf;

    set_user_nice(current, 10);
    adc_dev->flags =
	        DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
//    rx_dev = axi_adc_dev->rx_chan->device;
    /* RX can takes longer */
    adc_dev->rx_tmo = msecs_to_jiffies(AXI_ADC_CALLBACK_TIMEOUTMSEC);
    return ret;

  err_srcbuf:
    return -ENOMEM;
}

static void axi_adc_dma_start(struct dma_chan *chan)
{
    /*flush pending transactions to HW */
    dma_async_issue_pending(chan);
}

static void axi_adc_dma_stop(struct dma_chan *chan)
{
    struct dma_device *chan_dev;
    /* Terminate DMA transactions */
    if (chan) {
	     chan_dev = chan->device;
	     chan_dev->device_terminate_all(chan);
    }
}

static void axi_adc_slave_rx_callback(void *completion)
{
 

    complete(completion);
  
    
}

/* File operations */
int axi_adc_dma_open(struct inode *inode, struct file *filp)
{

    unsigned int mn;
    mn = iminor(inode);
    /*Assign minor number for later reference */
    filp->private_data = (void *) mn;
    return SUCCESS;
}

int axi_adc_dma_release(struct inode *inode, struct file *filp)
{
    return SUCCESS;
}

ssize_t axi_adc_dma_read(struct file * filep, char __user * buf,
			  size_t count, loff_t * f_pos)
{
  struct inode *inode = filep->f_inode;
  struct cdev  *chrdev = inode->i_cdev;
  struct axi_dma_adc_dev *adc_dev = container_of(chrdev, struct axi_dma_adc_dev, cdev);
  int    rx_tmo = 0, status = 0;
  static int  package_no =0;

    /* Validation for read size */
    if (count > AXI_ADC_BUFF_SIZE) {
	     dev_err(&adc_dev->pdev->dev, "improper buffer size \n");
	     return EINVAL;
    }

    /* Query minor number.
     * @To be extended for multiple channel support
     */
//    minor = (int) filep->private_data;
    rx_tmo = wait_for_completion_timeout(&adc_dev->rx_cmp,
				    adc_dev->rx_tmo);

    if (rx_tmo == 0) {
	     dev_err(&adc_dev->pdev->dev, "RX test timed out\n");
	     return -EAGAIN;
    }

    /*unmap one buffer to cpu from done_list*/
    dma_sync_single_for_cpu(adc_dev->rx_chan->device->dev,
		            adc_dev->dma_dsts+ package_no*AXI_ADC_BUFF_SIZE,
                            AXI_ADC_BUFF_SIZE, DMA_DEV_TO_MEM);    
    status = copy_to_user(buf, 
                          adc_dev->dsts+ package_no*AXI_ADC_BUFF_SIZE,
                          AXI_ADC_BUFF_SIZE);
    package_no++;
    if (package_no >= AXI_ADC_BUFF_NUM )
        package_no = 0;

    return AXI_ADC_BUFF_SIZE;
}

/* IOCTL calls provide interface to configure ,start and stop
   DMA engine */
static long axi_adc_dma_ioctl(struct file *filep,
			       unsigned int cmd, unsigned long arg)
{

    struct inode *inode = filep->f_inode;
    struct cdev  *chrdev = inode->i_cdev;
    struct axi_dma_adc_dev *adc_dev = container_of(chrdev, struct axi_dma_adc_dev, cdev);
//    struct xilinx_dma_config config;
    struct dma_device *rx_dev = NULL;
    struct dma_async_tx_descriptor *rxd = NULL;

    rx_dev = adc_dev->rx_chan->device;
    switch (cmd) {
    case AXI_ADC_DMA_CONFIG:
	/*Configures DMA transaction */

        printk(KERN_EMERG "enter ioctl--config \n"); 
	adc_dev->dma_dsts = dma_map_single(rx_dev->dev,
						adc_dev->dsts,
						AXI_ADC_BUFF_SIZE * AXI_ADC_BUFF_NUM,
						DMA_DEV_TO_MEM);
	/* Preparing mapped scatter-gather list */
//	sg_init_table(&axi_adc_dev->rx_sg, AXI_XADC_BUF_COUNT);
//	sg_dma_address(&axi_adc_dev->rx_sg) = axi_xadc_dev->dma_dsts;
	/* Configures S2MM data length */
//	sg_dma_len(&axi_adc_dev->rx_sg) = AXI_ADC_BUFF_SIZE;

	/* Only one interrupt */
//	config.coalesc = 1;
//	config.delay = 0;
//	rx_dev->device_control(axi_xadc_dev->rx_chan, DMA_SLAVE_CONFIG,
//			       (unsigned long) &config);
        printk(KERN_EMERG "pass ioctl--config step1 \n"); 
	/* Obtaining DMA descriptor */
	rxd = rx_dev->device_prep_dma_cyclic(adc_dev->rx_chan,
					   adc_dev->dma_dsts,
					   AXI_ADC_BUFF_SIZE * AXI_ADC_BUFF_NUM,
					   AXI_ADC_BUFF_SIZE,
					   DMA_DEV_TO_MEM,
					   adc_dev->flags);
	if (!rxd) {
	    dma_unmap_single(rx_dev->dev, adc_dev->dma_dsts,
			     AXI_ADC_BUFF_SIZE * AXI_ADC_BUFF_NUM, DMA_DEV_TO_MEM);
	    dev_err(&adc_dev->pdev->dev, "dma_buffer_map  error \n");
	    return -EIO;
	}
        printk(KERN_EMERG "pass ioctl--config step1 \n"); 
	init_completion(&adc_dev->rx_cmp);
	/* Added callback information */
	rxd->callback = axi_adc_slave_rx_callback;
	rxd->callback_param = &adc_dev->rx_cmp;
	/*Place transaction to DMA engine pending queue */
	adc_dev->rx_cookie = rxd->tx_submit(rxd);

	/* Check for dma submit errors */
	if (dma_submit_error(adc_dev->rx_cookie)) {
	    dev_err(&adc_dev->pdev->dev, "dma_submit error \n");
	    return -EIO;
	}
     printk(KERN_EMERG "leave ioctl--config  \n"); 
	break;
    case AXI_ADC_DMA_START:
	/* Start the DMA transaction */
        printk(KERN_EMERG "enter ioctl--dma start \n"); 
	      axi_adc_dma_start(adc_dev->rx_chan);
        printk(KERN_EMERG "leave ioctl--dma start \n"); 
	break;
    case AXI_ADC_DMA_STOP:
	/* Stop the DMA transaction */
         printk(KERN_EMERG "enter ioctl--dma stop \n"); 
	       axi_adc_dma_stop(adc_dev->rx_chan);
	       dma_unmap_single(rx_dev->dev, adc_dev->dma_dsts,
	       AXI_ADC_BUFF_SIZE * AXI_ADC_BUFF_NUM, DMA_DEV_TO_MEM);
         printk(KERN_EMERG "leave ioctl--dma stop \n");
	break;
    case AXI_ADC_ADSTART:
        iowrite32(ADSTART_ON,adc_dev->base+ADC_START_REG_OFFSET);  
        break; 
     case AXI_ADC_ADSTOP:
        iowrite32(ADSTART_OFF,adc_dev->base+ADC_START_REG_OFFSET);  
        break;  
    default:
       return -EOPNOTSUPP;

    }
    return SUCCESS;
}

struct file_operations axidma_adc_fops = {
    .owner = THIS_MODULE,
    .read = axi_adc_dma_read,
    .open = axi_adc_dma_open,
    .unlocked_ioctl = axi_adc_dma_ioctl,
    .release = axi_adc_dma_release
};

static const struct of_device_id axidma_adc_of_ids[] = {
    {.compatible = "xlnx,axidma-adc-1.0",},
    {} 
};
MODULE_DEVICE_TABLE(of, axidma_adc_of_ids);

static int axidma_adc_remove(struct platform_device *pdev)
{
    struct axi_dma_adc_dev *axidma_adc_dev = pdev->dev.driver_data;
                    
    cdev_del(&axidma_adc_dev->cdev);
    device_destroy(axidma_adc_class,
		MKDEV(MAJOR(axidma_adc_dev->devno),
		AXI_ADC_MINOR_START));

//    class_destroy(axi_adc_dev->axi_adc_class);
    unregister_chrdev_region(axidma_adc_dev->devno, AXI_ADC_MINOR_COUNT);

    kfree(axidma_adc_dev->dsts);

    /* Free up the DMA channel */
    dma_release_channel(axidma_adc_dev->rx_chan);

 
    if (axidma_adc_dev) {
	     kfree(axidma_adc_dev);
    }
    dev_info(&pdev->dev, "ADC DMA Unload :: Success \n");
    return SUCCESS;
}

static int axidma_adc_probe(struct platform_device *pdev)
{
    int status = 0, i = 0;
    const struct of_device_id *id;
    struct resource *mem;
    struct axi_dma_adc_dev *axidma_adc_dev;
	

    /*Allocate device node */
    if (!pdev->dev.of_node){
   	     dev_err(&pdev->dev, "unable to allocate device node\n");
	       return -ENODEV;
   	}

    id = of_match_node(axidma_adc_of_ids, pdev->dev.of_node);
    if (!id){
  	     dev_err(&pdev->dev, "unable to match device node \n");
	       return -ENODEV;
    }

  /* Allocate a private structure to manage this device */
    axidma_adc_dev = kmalloc(sizeof(struct axi_dma_adc_dev), GFP_KERNEL);
    if (!axidma_adc_dev ) {
	       dev_err(&pdev->dev, "unable to allocate device structure\n");
	       return -ENOMEM;
    }
    memset(axidma_adc_dev, 0, sizeof(struct axi_dma_adc_dev));

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    axidma_adc_dev->base = devm_ioremap_resource(&pdev->dev, mem);
    if (IS_ERR(axidma_adc_dev->base))
	      return PTR_ERR(axidma_adc_dev->base);

   status = of_property_read_string(pdev->dev.of_node,"dma-names",&axidma_adc_dev->chan_name);
   if (!status){
	     dev_err(&pdev->dev, "unable to find channel name\n");
		   goto fail1;
   }
   
    axidma_adc_dev->pdev = pdev;
    status =alloc_chrdev_region(&axidma_adc_dev->devno, 
		0, AXI_ADC_MINOR_COUNT,MODULE_NAME);
    if (status < 0) {
	      dev_err(&pdev->dev, "unable to alloc chrdev \n");
	      goto fail1;
    }

    /* Register with the kernel as a character device */
    cdev_init(&axidma_adc_dev->cdev, &axidma_adc_fops);
    axidma_adc_dev->cdev.owner = THIS_MODULE;
    axidma_adc_dev->cdev.ops = &axidma_adc_fops;


    status = cdev_add(&axidma_adc_dev->cdev, axidma_adc_dev->devno,
		         AXI_ADC_MINOR_COUNT);
    if (status){
         dev_err(&pdev->dev,"Err: failed in registering axidma_adc cdev.\n");  	
        goto fail1;
     }

    platform_set_drvdata(pdev, axidma_adc_dev);

    //Creating device node for each ADC channel
    device_create(axidma_adc_class, NULL,
		  MKDEV(MAJOR(axidma_adc_dev->devno), AXI_ADC_MINOR_START),
		  NULL, axidma_adc_dev->chan_name);

    dev_info(&pdev->dev, "Xilinx PL DMA_ADC added successfully\n");

    /* Initializing AXI DMA */
    axi_adc_dma_init(axidma_adc_dev);

    return SUCCESS;

    //Clean up
 
  fail1:
    kfree(axidma_adc_dev);
    return status;
}



static struct platform_driver axidma_adc_of_driver = {
    .driver = {
	       .name = MODULE_NAME,
	       .owner = THIS_MODULE,
	       .of_match_table = axidma_adc_of_ids,
	       },
    .probe = axidma_adc_probe,
    .remove = axidma_adc_remove,
};

//module_platform_driver(axi_adc_dma_of_driver);
static int __init axidma_adc_init(void)
{
	int status;

	/* Register a class that will key udev/mdev to add/remove /dev nodes.
	 *  Last, register the driver which manages those device numbers.
	 */

/* creating class to contain the device for the first time*/
  if(!axidma_adc_class){
    axidma_adc_class =class_create(THIS_MODULE, "AXI_DMA_ADC"); 
    if(IS_ERR(axidma_adc_class)) {
      printk("Err: failed in creating axidma_adc class.\n");
      return PTR_ERR(axidma_adc_class) ;
    }	
   }

  status = platform_driver_register(&axidma_adc_of_driver);
	if (status < 0) {
		class_destroy(axidma_adc_class);
	}
	return status;
}
module_init(axidma_adc_init);

static void __exit axidma_adc_exit(void)
{
	class_destroy(axidma_adc_class);
	platform_driver_unregister(&axidma_adc_of_driver);

}
module_exit(axidma_adc_exit);



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Xilinx AXI DMA ADC driver");
MODULE_AUTHOR("Xilinx, Inc.");
MODULE_VERSION("1.00");
