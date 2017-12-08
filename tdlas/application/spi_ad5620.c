 /*
 * Copyright (c) 2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "spi_ad5620.h"

#define AD5620_LEN 2
#define AD5620_SPEED 20000000
#define AD5620_DELAY 2
#define AD5620_BITs 8
#define SPI_DEV_AD5620 "/dev/spidev1.0"

#define AD5620_REF        (1250.0 * 2.0 )// ref volt = 1.25V , output amp gain = 2
#define AD5620_FULLSCALE       4096.0 // 12 bit




//------set AD5620 output for the desired temperature ----------------------//
int Laser_temp_set(double temp_volt)
{
    char buf[4];
    int file, binary_value,ret;

    file=ad5620_init(SPI_DEV_AD5620); //dev
    /*  AD5620 data format
     *  (MSB)PD1 PD0 D11 .. D0 x x
     *  PD1 PD0 =0b00; normal OP
     */
    binary_value = temp_volt * AD5620_FULLSCALE / AD5620_REF ;
    binary_value = (binary_value & 0xfff)<< 2;
    buf[1] = binary_value & 0xff;
    buf[0] = (binary_value & 0xff00) >> 8;
    ret = file;
    if(file >=0){
    	 if(file >0) ret = write(file,buf,AD5620_LEN);
       close(file);
    }

    return ret;
}



//////////
// Init SPIdev
//////////
int ad5620_init(char filename[AD5620_FILENAME_LEN])
{
int file;
__u8 mode=0, lsb, bits;
__u32 speed=AD5620_SPEED;
	if ((file = open(filename,O_WRONLY)) < 0)
	{
		printf("Failed to open the bus.");
		/* ERROR HANDLING; you can check errno to see what went wrong */
 		 return file;
	}

	// set SPI device parameters
	mode |= SPI_CPOL;/* CPOL=1, CPHA=0*/

	if (ioctl(file, SPI_IOC_WR_MODE, &mode) < 0)
	{
		perror("SPI rd_mode");
		return 0;
	}

	lsb = 0;/*AD5620 SPI is MSB */
	if (ioctl(file, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
	{
		perror("SPI rd_lsb_fist");
		return 0;
	}

	bits =8; /*AD5620 SPI is 16 bits */
	if (ioctl(file, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
	{
		perror("SPI bits_per_word");
		return 0;
	}

	if (ioctl(file, SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0)
	{
		perror("can't set max speed hz");
		return 0;
	}
	printf("write %s: spi mode %d, %d bits %sper word, %d Hz max\n",filename, mode, bits, lsb ? "(lsb first) " : "", speed);
///////////////
// Verifications
///////////////
//possible modes: mode |= SPI_LOOP; mode |= SPI_CPHA; mode |= SPI_CPOL; mode |= SPI_LSB_FIRST; mode |= SPI
//multiple possibilities using |

//	mode |= SPI_CPOL;/* CPOL=1, CPHA=0*/

//	if (ioctl(file, SPI_IOC_RD_MODE, &mode) < 0)
//	{
//		perror("SPI rd_mode");
//		return 0;
//	}

//	lsb = 0;/*AD5620 SPI is MSB */
//	if (ioctl(file, SPI_IOC_RD_LSB_FIRST, &lsb) < 0)
//	{
//		perror("SPI rd_lsb_fist");
//		return 0;
//	}

//	bits =8; /*AD5620 SPI is 16 bits */
//	if (ioctl(file, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0)
//	{
//		perror("SPI bits_per_word");
//		return 0;
//	}

//	if (ioctl(file, SPI_IOC_WR_MAX_SPEED_HZ, &speed)<0)
//	{
//		perror("can't set max speed hz");
//		return 0;
//	}

//	printf("read back %s: spi mode %d, %d bits %sper word, %d Hz max\n",filename, mode, bits, lsb ? "(lsb first) " : "", speed);

	return file;
}



