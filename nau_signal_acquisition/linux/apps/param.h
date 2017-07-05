#ifndef PARAM_H
#define PARAM_H


#define CMD_BYTEORDERCHECK	    			0x01
#define CMD_FILECHECK        						0x02
#define CMD_GET_PARAMETER    					0x03
#define CMD_GET_SWTABLE    						0x04
#define CMD_GET_DAWAVE        					0x05
#define CMD_SELFCHECK   								0x06
#define CMD_UPLOAD_PARAMETER        	0x07
#define CMD_UPLOAD_SWTABLE      			0x08
#define CMD_UPLOAD_DAWAVE       			0x09
#define CMD_UPLOAD_FILE							0x0A
#define CMD_REQUEST_FILE          		    	0x0B
#define CMD_FILE_NOMORE     					0x0C


#define SPI_DEV_LMK048 "/dev/spidev0.0"
#define SPI_DEV_AD9266 "/dev/spidev0.1"

#define BRAM0_BASE_ADDRESS     0x40000000
#define MAP_SIZE0 131072UL
#define MAP_MASK0  (MAP_SIZE0 - 1)
#define BRAM1_BASE_ADDRESS     0x42000000
#define MAP_SIZE1 65536UL
#define MAP_MASK1  (MAP_SIZE1 - 1)





#endif
