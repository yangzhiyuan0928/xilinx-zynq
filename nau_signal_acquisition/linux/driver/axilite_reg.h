#ifndef AXILITE_REG_H
#define AXILITE_REG_H

struct axilite_reg_dev{
        dev_t  devno;
        struct cdev cdev;
        struct device  *dev;
        int    irq;
	void   __iomem *regs;
	struct fasync_struct *async_queue;
};

struct class  * axilite_reg_class ;

#define AXILITE_REG_MINOR_START       0
#define AXILITE_REG_MINOR_COUNT       1

#define SUCCESS           	      0
#define FAILURE                      -1

#define MODULE_NAME  "axilite_reg"

#define DA_LENGTH_REG_OFFSET   0     
#define AD_LENGTH_REG_OFFSET   4  
#define TRIG_REG_OFFSET        8  
#define RSTN_REG_OFFSET        12  
#define LEDON_TIME_REG_OFFSET 16
#define LEDOFF_TIME_REG_OFFSET 20

#define AD_RSTN_mask           0x01
#define DA_RSTN_mask           0x02
#define TRIG_INT_mask          0x01
#define TRIG_SEL_mask          0x02

/* IOCTL defines */
#define AXILITE_IOCTL_BASE		'W'
#define AXILITE_RDALENGTH	        _IO(AXILITE_IOCTL_BASE, 0)
#define AXILITE_RADLENGTH		_IO(AXILITE_IOCTL_BASE, 1)
#define AXILITE_RTRIGINT		_IO(AXILITE_IOCTL_BASE, 2)
#define AXILITE_RTRIGSEL	        _IO(AXILITE_IOCTL_BASE, 3)
#define AXILITE_RADRSTN		        _IO(AXILITE_IOCTL_BASE, 4)
#define AXILITE_RDARSTN		        _IO(AXILITE_IOCTL_BASE, 5)
#define AXILITE_RLEDON_TIME	        _IO(AXILITE_IOCTL_BASE, 12)
#define AXILITE_RLEDOFF_TIME	        _IO(AXILITE_IOCTL_BASE, 13)

#define AXILITE_WDALENGTH		_IO(AXILITE_IOCTL_BASE, 6)
#define AXILITE_WADLENGTH		_IO(AXILITE_IOCTL_BASE, 7)
#define AXILITE_WTRIGINT		_IO(AXILITE_IOCTL_BASE, 8)
#define AXILITE_WTRIGSEL	        _IO(AXILITE_IOCTL_BASE, 9)
#define AXILITE_WADRSTN		        _IO(AXILITE_IOCTL_BASE, 10)
#define AXILITE_WDARSTN		        _IO(AXILITE_IOCTL_BASE, 11)
#define AXILITE_WLEDON_TIME	        _IO(AXILITE_IOCTL_BASE, 14)
#define AXILITE_WLEDOFF_TIME	        _IO(AXILITE_IOCTL_BASE, 15)

#endif
