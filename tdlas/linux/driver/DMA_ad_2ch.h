/*
 * AXI DMA driver.
 * Configures ADC in continuous stream and DMA in SG mode.
 * Uses DMA Client API to configure Xilinx DMA engine. 
 * Provide interface to read ADC Samples.
 * Copyright (C) 2012 Xilinx, Inc.
 * Copyright (C) 2012 Robert Armstrong
 *
 * Author: Radhey Shyam Pandey <radheys@xilinx.com>
 * 
 * Based on PL330 DMA Driver.
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

#ifndef AXIDMA_ADC_H
#define AXIDMA_ADC__H

#define MODULE_NAME             	"axidma_adc"
#define AXI_ADC_MINOR_START           0
#define AXI_ADC_MINOR_COUNT         	1
#define AXI_ADC_CALLBACK_TIMEOUTMSEC 	10000
#define AXI_ADC_BUFF_SIZE              4096
#define AXI_ADC_BUFF_NUM               32
#define SUCCESS                 	     0
#define FAILURE                        -1
#define AXI_XADC_BUF_COUNT              1


#define S2MM_COMPLETION_MASK         GENMASK(31,31)
#define RECEIVED_BYTES_MASK          GENMASK(22, 0) 



/* IOCTL defines */
#define AXI_ADC_IOCTL_BASE		'W'
#define AXI_ADC_GET_NUM_DEVICES	        _IO(AXI_ADC_IOCTL_BASE, 0)
#define AXI_ADC_GET_DEV_INFO		_IO(AXIXADC_IOCTL_BASE, 1)
#define AXI_ADC_DMA_CONFIG		_IO(AXI_ADC_IOCTL_BASE, 2)
#define AXI_ADC_DMA_START		_IO(AXI_ADC_IOCTL_BASE, 3)
#define AXI_ADC_DMA_STOP		_IO(AXI_ADC_IOCTL_BASE, 4)
#define AXI_ADC_ADSTART		        _IO(AXI_ADC_IOCTL_BASE, 5)
#define AXI_ADC_ADSTOP		        _IO(AXI_ADC_IOCTL_BASE, 6)

struct axi_dma_adc_dev {
    dev_t devno;
    void   __iomem    *base;
    struct mutex mutex;
    struct cdev cdev;
    struct platform_device *pdev;
    /* DMA stuff */
    struct dma_chan *rx_chan;
    dma_cookie_t rx_cookie;
    struct completion rx_cmp;
    unsigned long rx_tmo;
//    struct scatterlist rx_sg;
    /*DMA address of buffer */
    dma_addr_t dma_dsts;
    enum dma_ctrl_flags flags;
    u8 *dsts;
    const char *chan_name;
    int package_no;
};





#endif
