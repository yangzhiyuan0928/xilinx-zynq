#ifndef AXIGPIO_REG_H
#define AXIGPIO_REG_H

struct axi_gpio_dev{
        dev_t  devno;
        struct cdev cdev;
        struct device  *dev;
        int    irq;
        bool   wait_flag;
	void   __iomem *regs;
};

struct class  * axigpio_class ;

#define AXIGPIO_MINOR_START           0
#define AXIGPIO_MINOR_COUNT           1

#define SUCCESS           	      0
#define FAILURE                      -1

#define MODULE_NAME  "z7nau_axigpio"

#define Z7REG_GPIODATA_OFFSET     0x0     
#define Z7REG_GPIOTRI_OFFSET      0x4  
#define Z7REG_GPIO2DATA_OFFSET    0x8  
#define Z7REG_GPIO2TRI_OFFSET     0x0c
#define Z7REG_GIER_OFFSET         0x011c
#define Z7REG_IPIER_OFFSET        0x0128 
#define Z7REG_IPISR_OFFSET        0x0120

#define KEY_INPUT_MASK            0x01

#define Z7GPIO_DIR                0x0000   //ALL output
#define Z7GPIO2_DIR               0x0001   //ONLY 1 input
#define Z7GPIO_GIER               0x8000   //GLOBAL INTERRUPT ENABLE
#define Z7GPIO_IER_MASK           0x0002   //interrupt enable, only GPIO2
#define Z7GPIO_ISR_MASK           0x0002   //interrupt STATUS, only GPIO2
#define Z7GPIO_GIER_MASK          0x0001 



/* IOCTL defines */
#define Z7AXIGPIO_IOCTL_BASE		'W'
#define Z7AXIGPIO_RDATA	        _IO(Z7AXIGPIO_IOCTL_BASE, 20)
#define Z7AXIGPIO_RTRI		_IO(Z7AXIGPIO_IOCTL_BASE, 21)
#define Z7AXIGPIO2_RDATA	_IO(Z7AXIGPIO_IOCTL_BASE, 22)
#define Z7AXIGPIO2_RTRI	        _IO(Z7AXIGPIO_IOCTL_BASE, 23)
#define Z7AXIGPIO_RGIER 	_IO(Z7AXIGPIO_IOCTL_BASE, 24)
#define Z7AXIGPIO_RIER		_IO(Z7AXIGPIO_IOCTL_BASE, 25)
#define Z7AXIGPIO_RISR		_IO(Z7AXIGPIO_IOCTL_BASE, 26)

#define Z7AXIGPIO_WDATA	        _IO(Z7AXIGPIO_IOCTL_BASE, 27)
#define Z7AXIGPIO_WTRI		_IO(Z7AXIGPIO_IOCTL_BASE, 28)
#define Z7AXIGPIO2_WDATA	_IO(Z7AXIGPIO_IOCTL_BASE, 29)
#define Z7AXIGPIO2_WTRI	        _IO(Z7AXIGPIO_IOCTL_BASE, 30)
#define Z7AXIGPIO_WGIER 	_IO(Z7AXIGPIO_IOCTL_BASE, 31)
#define Z7AXIGPIO_WIER		_IO(Z7AXIGPIO_IOCTL_BASE, 32)
#define Z7AXIGPIO_WISR		_IO(Z7AXIGPIO_IOCTL_BASE, 33)

#endif
