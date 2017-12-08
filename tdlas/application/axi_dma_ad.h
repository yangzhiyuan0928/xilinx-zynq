/*
 * axi_dma_ad.h
 *
 *  Created on: Aug 19, 2016
 *      Author: owbama
 */

#ifndef AXI_DMA_AD_H_
#define AXI_DMA_AD_H_

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

/* Opaque buffer element type.  This would be defined by the application. */
typedef struct { unsigned char buf[FIFO_SIZE+4]; } ElemType;

struct  thread_param_t{
	ElemType * buf_p;
	const char *devfile_name;
	bool  flag_ad;
	bool  capture_start;
	bool  chan_st;
	pthread_t t_datacapture;
	unsigned short  port_n;
	const  char *server_ip;
};


int captureADCSamples(void *ptr);
int dma_ad_start(struct thread_param_t *thread_parameter);
int dma_ad_stop(struct thread_param_t *thread_parameter);

ssize_t readn(int fd, void *buffer, size_t n);
ssize_t writen(int fd, void *buffer, size_t n);

#endif /* AXI_DMA_AD_H_ */
