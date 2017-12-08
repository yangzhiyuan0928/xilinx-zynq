#ifndef AXI_DEFINES_H
#define AXI_DEFINES_H
/* IOCTL defines */

/* IOCTL defines */
#define AXI_ADC_IOCTL_BASE		'W'
#define AXI_ADC_GET_NUM_DEVICES	        _IO(AXI_ADC_IOCTL_BASE, 0)
#define AXI_ADC_GET_DEV_INFO		_IO(AXIXADC_IOCTL_BASE, 1)
#define AXI_ADC_DMA_CONFIG		_IO(AXI_ADC_IOCTL_BASE, 2)
#define AXI_ADC_DMA_START		_IO(AXI_ADC_IOCTL_BASE, 3)
#define AXI_ADC_DMA_STOP		_IO(AXI_ADC_IOCTL_BASE, 4)
#define AXI_ADC_ADSTART		        _IO(AXI_ADC_IOCTL_BASE, 5)
#define AXI_ADC_ADSTOP		        _IO(AXI_ADC_IOCTL_BASE, 6)

#define FIFO_SIZE ((4*1024))

#define XST_FAILURE -1
#define XST_SUCCESS  0

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#endif
