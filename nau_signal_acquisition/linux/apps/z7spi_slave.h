
#ifndef  SPI_SALVE_Z7NAU_H
#define  SPI_SALVE_Z7NAU_H


#include <linux/types.h>


struct SPI_Config{
       __u8 mode;
       __u8 lsb;
       __u8 bits;
       __u32 speed;
  };


int SPIslave_Config(char *filename,struct SPI_Config spiconfig,char *tx_buffer,int count,char *rx_buffer);
int SPICtl_Init(char* filename,struct SPI_Config spiconfig);


#endif /*SPI_SALVE_Z7NAU_H*/
