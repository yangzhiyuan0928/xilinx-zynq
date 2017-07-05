

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/spi/spidev.h>
#include "z7spi_slave.h"


#define Z7_SPI_DELAY  100

int SPIslave_Config(char *filename,struct SPI_Config spiconfig,char *tx_buffer,int count,char *rx_buffer)
{
    int file,ret;

    struct spi_ioc_transfer tr = {   
		.tx_buf = (unsigned long)tx_buffer,
		.rx_buf = (unsigned long)rx_buffer,
		.len = count,
		.delay_usecs = Z7_SPI_DELAY,
		.speed_hz = spiconfig.speed,
		.bits_per_word = spiconfig.bits,
	};

	file=SPICtl_Init(filename,spiconfig);
        if(file < 0)
		return file;

    
     ret = ioctl(file, SPI_IOC_MESSAGE(1), &tr);  
	if (ret < 1)
		return -1;

	//for (ret = 0; ret < count; ret++) {
	//	printf("receive data rx_buffer[%d]: %.2X\n",ret, rx_buffer[ret]);
	//}

     
     close(file);
     
    return ret;
}



int SPICtl_Init(char* filename,struct SPI_Config spiconfig)
{
int file;

	if ((file = open(filename,O_WRONLY)) < 0)     
	{
		printf("Failed to open the bus.");
 		 return file;
	}

	// set SPI device parameters


	if (ioctl(file, SPI_IOC_WR_MODE, &spiconfig.mode) < 0)
	{
		perror("SPI rd_mode");
		return 0;
	}


	if (ioctl(file, SPI_IOC_WR_LSB_FIRST, &spiconfig.lsb) < 0)
	{
		perror("SPI rd_lsb_fist");
		return 0;
	}


	if (ioctl(file, SPI_IOC_WR_BITS_PER_WORD, &spiconfig.bits) < 0)
	{
		perror("SPI bits_per_word");
		return 0;
	}

	if (ioctl(file, SPI_IOC_WR_MAX_SPEED_HZ, &spiconfig.speed)<0)
	{
		perror("can't set max speed hz");
		return 0;
	}
	
	return file;
}
